#!/usr/bin/ruby
require 'pry'
require 'httparty'
require 'nokogiri'

class Automate
	attr_accessor :headers
	attr_accessor :cookies
	attr_reader :response
	def initialize
		@cookies = HTTParty::CookieHash.new
		@headers = {}
		@headers['User-Agent'] = 'Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 10.0; WOW64; Trident/7.0; .NET4.0C; .NET4.0E)'
		@headers['Accept-Encoding'] = ''
	end
	def login
		@response = HTTParty.post(
			'http://192.168.10.1/netview/libx/HOME',
			:body => {login: 'user', passwd: 'password'},
			:headers => @headers,
			:follow_redirects => false
		)
		@response.get_fields('Set-Cookie').each { |c|
			cookies.add_cookies(c)
		}
		unless cookies[:sessionid].nil? || cookies[:sessionid].empty?
			puts "Got sessionid cookie"
		else
			puts "Login failed, no sessionid cookie!"
			exit 1
		end
		if @response.code == 303
			puts "Login succeeded"
		else
			puts "Login failed, no redirect!"
			exit 1
		end
	end
	# returns array of invoice numbers for the month
	def request_posh(month = '00')
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
			year: '2020',
			invno: '',
			ptno: '',
			pono: '',
		}
		invoices = []
		page_count = 0
		loop do
			page_count += 1
			print "Getting page of invoices... "
			next_data, page_invoices = request_posh_page(data, month)
			next_data[:searchMode] = 'X' #after the first page use searchMode = X
			invoices += page_invoices
			puts "#{page_invoices.count} invoices"
			if data[:startPos] == next_data[:startPos]
				puts "Last page, total #{page_count} pages."
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
			'http://192.168.10.1/netview/in/POSHdata',
			:body => posh_post_data,
			:headers => @headers,
			:follow_redirects => false
		)
		@posh_body = Nokogiri::HTML(@response.body) {|c| c.noblanks }
		#puts @response.body
		#select all POSH records: @posh_body.xpath('//span[@class="oline"]|//span[@class="eline"]')
		lines = @posh_body.xpath('//span[@class="eline"]|//span[@class="oline"]')
		poshes = []
		lines.each do |line|
			poshes << POSH_entry.new(line)
		end

		# vvv Returns an array of invoice numbers from this page of POSH records, as text. vvv
		#invoice_numbers = @posh_body.xpath('//span[@class="invno"]').map {|x| x.children.text }


		#populate posh_post_data for the Next button for pagination
		form_inputs = @posh_body.xpath('//form[@id="main"]/input')
		next_data = {}
		form_inputs.each do |inp|
			#map 'em to a hash for posh_post_data and return it for next calll
			next_data.store(inp['name'].to_sym, inp['value'])
		end

		return [next_data, poshes]
	end
end

class POSH_entry
	attr_reader :slsman, :cstdtl, :tikno, :piktik, :invno, :invdte,
		:amtdtl, :custno, :invdt, :piktyp, :rprog_c, :sig

	#pass nokogiri object of eline/oline to this function
	def initialize(n)
		id = n['id'].gsub(/^dtl/, '')
		@slsman = n.at_css('.slsman').content
		@cstdtl = n.at_css('.cstdtl').content.to_i
		@tikno = @piktik = n.at_css('.piktik').content.to_i
		@invno = n.at_css('.invno').content.to_i
		@invdte = n.at_css('.invdte').content
		@amtdtl = n.at_css('.amtdtl').content
		@custno = n.at_css("#custno#{id}").content.to_i
		@invdt = n.at_css("#invdt#{id}").content
		@piktyp = n.at_css("#piktyp#{id}").content
		@rprog_c = n.at_css("#rprog_c#{id}").content
		@sig = n.at_css("#sig#{id}").content
	end

	def get_ticket_preview
		url = '/netview/in/POSHdisp'
		request = "GET #{url}?loc1+132259+23675+08032020+15697+20017+user"
	end

	def delete_ticket_preview
		timestring = URI::escape(Time.now.strftime("%a %b %d %H:%M:%S %Y"))
		url='/netview/libx/rmfile'
		request = "GET #{url}?#{rptPath}+#{timestring}"
		puts "TODO: #{request}"
	end
end

bot = Automate.new
bot.login
posh_list = bot.request_posh('08')
puts "Found #{invoice_list.count} invoices"
puts invoice_list
#from results:
#select span#eline and span#oline
#
#
#GET /netview/in/POSHdisp?loc1+132259+23675+08032020+15697+20017+user
#GET /netview/libx/rmfile?../../loc1/pikdir/pik15697.out+Wed%20Aug%2012%2010:17:41%202020
