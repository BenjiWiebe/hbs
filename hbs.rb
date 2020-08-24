#!/usr/bin/ruby
require 'pry'
require 'httparty'
require 'nokogiri'
require 'progress_bar'
require 'logger'
require 'terminal-table'
require 'bigdecimal'

$DEBUG = true

if $DEBUG
	require 'pry'
end

$log = Logger.new(STDOUT)
$log.level = Logger::WARN

class Automate
	attr_accessor :headers
	attr_accessor :cookies
	attr_reader :response
	HBSIP = '192.168.10.1'
	HBSURL = "http://#{HBSIP}"
	def initialize
		@cookies = HTTParty::CookieHash.new
		@headers = {}
		@headers['User-Agent'] = 'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 10.0; WOW64; Trident/7.0; .NET4.0C; .NET4.0E)'
		@headers['Accept-Encoding'] = ''
	end
	def login
		@response = HTTParty.post(
			"#{HBSURL}/netview/libx/HOME",
			:body => {login: 'user', passwd: 'password'},
			:headers => @headers,
			:follow_redirects => false
		)
		@response.get_fields('Set-Cookie').each { |c|
			@cookies.add_cookies(c)
		}
		unless @cookies[:sessionid].nil? || @cookies[:sessionid].empty?
			$log.info "Got sessionid cookie"
			@headers['Cookie'] = @cookies.to_cookie_string
		else
			$log.error "Login failed, no sessionid cookie!"
			exit 1
		end
		if @response.code == 303
			$log.info "Login succeeded"
		else
			$log.error "Login failed, no redirect!"
			exit 1
		end
	end

	def start_posh
		@response = HTTParty.get(
			"#{HBSURL}/netview/in/POSH?loc1+user+",
			:headers => @headers,
			:follow_redirects => false
		)
		@posh_body = Nokogiri::HTML(@response.body) {|c| c.noblanks }
		@poshpid = @posh_body.at_css("input#poshPid")['value']
		$log.info "Using poshPid = #{@poshpid}"
	end

	# returns array of POSH_entry's for the month
	# use month code 00 to get a year's worth
	def request_posh(month = '00', year = '2020')
		data = {
			custno: '',
			curloc: 'loc1',
			startPos: 0,
			lastPage: '0',
			searchMode: 'N',
			fileAction: 'bldLST',
			srchType: 'd',
			custname: '',
			month: month,
			invdate: '',
			year: year,
			invno: '',
			ptno: '',
			pono: '',
		}
		invoices = []
		page_count = 0
		loop do
			page_count += 1
			next_data, page_invoices = request_posh_page(data, month)
			next_data[:searchMode] = 'X' #after the first page use searchMode = X
			invoices += page_invoices
			$log.info "Got page of #{page_invoices.count} invoices"
			if page_invoices.count < 15
				$log.info "Last page, total #{page_count} pages."
				break
			end
			#NOTE Use logic from HBS code to determine last page!!!
			#(I don't know how!!)
			#if data[:lastPage].to_i < 15
			#	puts "Last page, total #{page_count} pages."
			#	break
			#end
			if data[:startPos] == next_data[:startPos]
				# Warning because generally the above if statement should be the last page detector.
				# I have no idea what HBS actually does when the last page has exactly 15 invoices.
				$log.warn "Last page, total #{page_count} pages. Used startPos logic!"
				break
			end
			data = next_data
		end
		return invoices
	end
	
	# returns [next startPos, array of invoice numbers]
	def request_posh_page(posh_post_data, month = '00')
		@response = nil
		@response = HTTParty.post(
			"#{HBSURL}/netview/in/POSHdata",
			:body => posh_post_data,
			:headers => @headers,
			:follow_redirects => false
		)
		@posh_body = Nokogiri::HTML(@response.body) {|c| c.noblanks }
		#puts @response.body
		#select all POSH records: @posh_body.xpath('//span[@class="oline"]|//span[@class="eline"]')
		#if @style exists, then it isnt an empty line.
		lines = @posh_body.xpath('//span[@class="eline" and @style]|//span[@class="oline" and @style]')
		poshes = []
		lines.each do |line|
			poshes << POSH_entry.new(line)
		end

		#populate posh_post_data for the Next button for pagination
		form_inputs = @posh_body.xpath('//form[@id="main"]/input')
		next_data = {}
		form_inputs.each do |inp|
			#map 'em to a hash for posh_post_data and return it for next calll
			next_data.store(inp['name'].to_sym, inp['value'])
		end

		return [next_data, poshes]
	end

	# Returns array of invoice lines
	def get_ticket_preview(p)
		url = "#{HBSURL}/netview/in/POSHdisp?loc1+#{p.tikno}+#{p.custno}+#{p.invdt}+#{@poshpid}+#{p.invno}+user"
		$log.info "Using GET: #{url}"
		@response = HTTParty.get(
			url,
			:headers => @headers,
			:follow_redirects => false
		)
		if @response.code == 200
			poshdisp = Nokogiri::HTML(@response.body) {|c| c.noblanks }
			hbs_errmsg = poshdisp.at_css("input#errmsg")
			if hbs_errmsg != nil && hbs_errmsg['value'] != nil && hbs_errmsg['value'].length > 0
				$log.error "Error generating preview!"
				$log.error "Ticket ##{p.tikno}, Invoice ##{p.invno}"
				$log.error "HBS error: #{hbs_errmsg['value']}"
				raise HBSErrException.new(hbs_errmsg)
			end
			close_form = poshdisp.at_xpath('//span/input[@id="rptPath"]/parent::span')
			rptPath = close_form.at_css('input#rptPath')['value']
			$log.info "Generated preview #{File.basename(rptPath)}"
			delete_ticket_preview(rptPath)
			invspan = poshdisp.at_css('span#see')
			invlines = invspan.children.map {|child| child.content }
			return invlines
		else
			$log.error "Error generating preview!"
		 	$log.error "Ticket ##{p.tikno}, Invoice ##{p.invno}"
			return nil
		end
	end

	def delete_ticket_preview(rptPath)
		timestring = URI::escape(Time.now.strftime("%a %b %d %H:%M:%S %Y"))
		url = "#{HBSURL}/netview/libx/rmfile?#{rptPath}+#{timestring}"
		@response = HTTParty.get(
			url,
			:headers => @headers,
			:follow_redirects => false
		)
		pt = File.basename(rptPath)
		if @response.code == 200
			$log.info "Deleted preview #{pt}"
		else
			$log.warn "Deleting preview #{pt} failed: HTTP Code #{@reponse.code}"
		end
	end

end

class POSH_entry
	attr_reader :slsman, :tikno, :piktik, :invno, :invdte,
		:amtdtl, :custno, :invdt, :piktyp, :rprog_c, :sig, :status
	attr_accessor :sessid

	#pass nokogiri object of eline/oline to this function
	def initialize(n)
		id = n['id'].gsub(/^dtl/, '')
		@slsman = n.at_css('.slsman').content
		@tikno = @piktik = n.at_css('.piktik').content.to_i
		@invno = n.at_css('.invno').content
		@invdte = n.at_css('.invdte').content
		@amtdtl = n.at_css('.amtdtl').content
		@custno = n.at_css("#custno#{id}").content
		@invdt = n.at_css("#invdt#{id}").content
		@piktyp = n.at_css("#piktyp#{id}").content
		@rprog_c = n.at_css("#rprog_c#{id}").content
		@sig = n.at_css("#sig#{id}").content
		@status = n.at_css('.status').content
		@status.gsub!(/[[:space:]]/, '') # Remove whitespace
		@status.gsub!(/&nbsp/, '')
		if @status.length > 0
			if @status == "VOID"
				$log.info "Found voided invoice ##{@invno}"
			else
				$log.warn "POSH entry has an unknown status of #{@status}"
			end
		end
		if @tikno == 0
			$log.warn "POSH entry has a tikno of 0"
		end
	end

end

class SalesTaxEntry
	attr_accessor :invno, :amount, :taxamount, :taxable
	def initialize(posh_entry = nil, invlines = nil)
		@invno = posh_entry.invno
		if invlines == nil
			@amount = BigDecimal("0")
			@taxamount = BigDecimal("0")
			@taxable = BigDecimal("0")
			return
		end
		if posh_entry.status == "VOID"
			$log.info "Invoice ##{posh_entry.invno} is void, setting totals to 0."
			@amount = BigDecimal("0")
			@taxamount = BigDecimal("0")
			@taxable = BigDecimal("0")
			return
		elsif posh_entry.status.length > 0
			$log.warn "Invoice ##{posh_entry.invno} has unknown status of #{posh_entry.status}"
		end
		begin
			tax=invlines.find {|e| /SALES[[:space:]]TAX/ =~ e }
			tax = tax[tax.index('X')+1..tax.length] #get substring from after the X of TAX, until end of string
			tax.gsub!(/[[:space:]]/,'') #Remove whitespace
			taxable = invlines.find {|e| /^[[:space:]]*TAXABLE/ =~ e }
			taxable = taxable[taxable.index('E')+1..taxable.length] # Get substr from after text
			taxable.gsub!(/[[:space:]]/,'') #Remove whitespace
			@taxamount = @taxable = 0
			@taxamount = BigDecimal(tax) unless tax.nil?
			@taxable = BigDecimal(taxable) unless taxable.nil?
			@amount = BigDecimal(posh_entry.amtdtl)
		rescue Exception => e
			$log.error "Fatal error finding tax entry in invoice"
			binding.pry if $DEBUG
			raise e unless $DEBUG
		end
	end
end

class Array
	include ProgressBar::WithProgress
end

class HBSErrException < Exception
end

# convert a BigDecimal to a string formatted as dollars
def bd_to_usd(bd)
	return "$" + (bd.truncate(2).to_s("F") + "00")[ /.*\..{2}/ ]
end

bot = Automate.new
bot.login
bot.start_posh
posh_list = bot.request_posh('00', 2019)
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
