#!/usr/bin/ruby
require 'pry'
require 'httparty'
require 'nokogiri'
require 'progress_bar'
require 'logger'
require 'terminal-table'
require 'bigdecimal'
require 'base64'
require 'yaml'

require_relative 'lib.rb'

puts "WARNING: Use WARRANTY line when existing!"
puts "Invoice #20883"
exit 1
$DEBUG = $ARGV.delete("--debug") ? true : false

$log = Logger.new(STDOUT)
$log.level = Logger::WARN
$log.level = Logger::INFO if $ARGV.delete("--verbose")

require 'pry' if $DEBUG

# convert a BigDecimal to a string formatted as dollars
def bd_to_usd(bd)
	return "$" + (bd.truncate(2).to_s("F") + "00")[ /.*\..{2}/ ]
end

cfgfile = 'hbs.config'
cfg = YAML.load(File.open(cfgfile).read)
loc = cfg["loc"].nil? ? "loc1" : cfg["loc"]
user = cfg["user"]
password = cfg["password"]
ip = cfg["ip"].nil? ? '192.168.10.1' : cfg["ip"]

if user.nil? || password.nil?
	$log.error "Username and password are required."
	exit 1
end

bot = Automate.new(user, Base64.decode64(password), ip, loc)
bot.login
bot.start_posh
posh_list = bot.request_posh('09')
puts "Found #{posh_list.count} invoices"
puts "Getting all sales tax amounts"

stes = []
totalsales = BigDecimal("0")
posh_list.each_with_progress do |poshitem|
	begin
		inv = bot.get_ticket_preview(poshitem)
	rescue HBSErrException => e
		$log.error "HBS Error on invoice ##{poshitem.invno}: #{e.message}"
		$log.error "Not using taxable amounts"
		totalsales += SalesTaxEntry.new(poshitem).amount
		next
	end
	ste = SalesTaxEntry.new(poshitem, inv)
	stes << ste if ste.taxamount != 0
	totalsales += ste.amount
end

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
