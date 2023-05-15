#!/usr/bin/ruby
require 'pry'
require 'httparty'
require 'nokogiri'
require 'progress_bar'
require 'logger'
require 'terminal-table'
require 'bigdecimal'
require 'base64'

require_relative 'lib.rb'

MONTH='04'
YEAR='2023'
SAVE_LOG_TO="#{MONTH}-#{YEAR}-hbs_log.txt"
SEARCH_TERM=nil
#SEARCH_TERM=/searchterm/

do_save = SAVE_LOG_TO.to_s.empty? ? false : true

$DEBUG = $ARGV.delete("--debug") ? true : false

$log = Logger.new(STDOUT)
$log.level = Logger::WARN
$log.level = Logger::INFO if $ARGV.delete("--verbose")

require 'pry' if $DEBUG

# convert a BigDecimal to a string formatted as dollars
def bd_to_usd(bd)
	return "$" + (bd.truncate(2).to_s("F") + "00")[ /.*\..{2}/ ]
end

$config = HBSConfig.new('hbs.config')
if $config.user.nil? || $config.password.nil?
	$log.error "Username and password are required."
	exit 1
end

bot = Automate.new($config.user, Base64.decode64($config.password), $config.ip, $config.loc)
bot.login
bot.start_posh
posh_list = bot.request_posh(MONTH, YEAR)
puts "Found #{posh_list.count} invoices"
puts "Getting all sales tax amounts"

stes = []
totalsales = BigDecimal("0")
file = File.open(SAVE_LOG_TO, "w") if do_save
posh_list.each_with_progress do |poshitem|
	invtype = :normal
	begin
		inv = bot.get_ticket_preview(poshitem)
		piktikline = inv[10,1].first

		# Check if its a INTERNAL or WARRANTY ticket
		if piktikline.match?(/PIK[[:space:]]TIK[[:space:]]NO/)
		parts = piktikline.split(/[[:space:]]+/)
			if parts[5] != "T/S:"
				puts "WARNING: T/S not found on invoice ##{poshitem.invno}"
			else
				if parts[6] == "WARRANTY"
					invtype = :warranty
				elsif parts[6] == "INTERNAL"
					invtype = :internal
				end
			end
		end
		if do_save
			file.write(inv.join("\n"))
			file.write("\n\n\n")
		end
	rescue HBSErrException => e
		$log.error "HBS Error on invoice ##{poshitem.invno}: #{e.message}"
		$log.error "Not using taxable amounts"
		totalsales += SalesTaxEntry.new(poshitem).amount
		next
	end
	if SEARCH_TERM
		results = inv.select {|line| line =~ SEARCH_TERM }
		if results.length > 0
			puts "Found invoice ##{poshitem.invno} matching #{SEARCH_TERM.inspect}"
		end
	end
	if invtype == :normal
		ste = SalesTaxEntry.new(poshitem, inv)
		stes << ste if ste.taxamount != 0
		totalsales += ste.amount
	else
		puts "Found ticket of type #{invtype.to_s.upcase}: ##{poshitem.invno}"
	end
end

file.close if do_save

puts "Found #{stes.count} taxable invoices."

total_tax = stes.map {|t| t.taxamount }.sum
total_amount = stes.map {|t| t.amount }.sum
taxable_amount = stes.map {|t| t.taxable }.sum

tax_str = bd_to_usd(total_tax)
amt_str = bd_to_usd(total_amount)
taxamt_str = bd_to_usd(taxable_amount)
totalsales_str = bd_to_usd(totalsales)

rows = []
stes.each do |s|
	a = bd_to_usd(s.amount)
	t = bd_to_usd(s.taxable)
	ta = bd_to_usd(s.taxamount)
	rows << [s.invno, a, t, ta]
end

table = Terminal::Table.new :headings => ['Invoice#', 'Amount', 'Taxable', 'Tax'], :rows => rows
puts table

puts
puts
puts "Total tax: #{tax_str}"
puts "Total taxable: #{taxamt_str}"
puts "Total sales for the month: #{totalsales_str}"
