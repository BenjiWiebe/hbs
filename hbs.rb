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

		# vvv Returns an array of invoice numbers from this page of POSH records, as text. vvv
		invoice_numbers = @posh_body.xpath('//span[@class="invno"]').map {|x| x.children.text }


		#populate posh_post_data for the Next button for pagination
		form_inputs = @posh_body.xpath('//form[@id="main"]/input')
		next_data = {}
		form_inputs.each do |inp|
			#map 'em to a hash for posh_post_data and return it for next calll
			next_data.store(inp['name'].to_sym, inp['value'])
		end

		return [next_data, invoice_numbers]
	end
end

bot = Automate.new
bot.login
invoice_list = bot.request_posh
puts "Found #{invoice_list.count} invoices"
#from results:
#select span#eline and span#oline
#custno=&curloc=loc1&startPos=9315520&lastPage=15&searchMode=X&fileAction=bldLST&srchType=d&custname=&month=00&invdate=&year=2020&invno=&ptno=&pono=
#custno=&curloc=loc1&startPos=9316720&lastPage=15&searchMode=X&fileAction=bldLST&srchType=d&custname=&month=00&invdate=&year=2020&invno=&ptno=&pono=
#custno=&curloc=loc1&startPos=9317920&lastPage=15&searchMode=X&fileAction=bldLST&srchType=d&custname=&month=00&invdate=&year=2020&invno=&ptno=&pono=
#
