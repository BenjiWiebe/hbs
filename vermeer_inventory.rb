#!/usr/bin/ruby
require 'json'
require 'pry'
require 'net/smtp'
require 'base64'
require 'yaml'

cfgfile = 'vermeer_inventory.config'
cfg = YAML.load(File.open(cfgfile).read)
from_addr = cfg["from"]
from_name = cfg["from_name"]
password = cfg["password"]
smtp_server = cfg["smtp_server"]
to_addr = cfg["to"]
status_to_addr = cfg["status_to"]
dealer_id = cfg["dealer_id"]
vendor_id = cfg["vendor_id"]
if [from_addr,from_name,password,smtp_server,to_addr,status_to_addr,dealer_id].contains?(nil)
	raise "Make sure all configuration fields are populated!"
end


cmd = "/home/benji/invent -j --match-vendor #{vendor_id}"
results = JSON.parse(`#{cmd}`)
file = Tempfile.new('vermeer')
stocked_parts = results.count
negative_parts = []
File.open(file, "w") do |f|
	results.each do |r|
		if(r["onhand"] > 0)
			f.write "#{r["partnumber"]},#{r["onhand"]}\n"
		else
			stocked_parts -= 1
			negative_parts << r if(r["onhand"] < 0)
		end
	end
end


status_msg = <<END
From: #{from_name} <#{from_addr}>
To: <#{to_addr}>
Subject: Vermeer inventory update report

Sent parts inventory list to Vermeer. #{stocked_parts} stocked parts.
Following is a list of parts with negative on hand values:

END
negative_parts.each do |p|
	status_msg += "#{p["partnumber"]}: #{p["onhand"]}\n"
end

boundary = '------ATTACH'
message = <<END
From: #{from_name} <#{from_addr}>
To: <#{to_addr}>
Subject: #{dealer_id}
MIME-Version: 1.0
Content-Type: multipart/mixed; boundary=#{boundary}
--#{boundary}

Inventory update is attached.
--#{boundary}
Content-Type: text/plain; charset=UTF-8; name="#{dealer_id}.csv"
Content-Transfer-Encoding: base64
Content-Disposition: attachment; filename="#{dealer_id}.csv"

END

message += Base64.encode64(File.read(file))
message += "--" + boundary

password='S2U7NjBUMEUkdSF7Q0ZxW2tDc0o='
smtp = Net::SMTP.new smtp_server, 587
smtp.enable_starttls
smtp.start('localhost',from_addr,Base64.decode64(password), :login) do
	smtp.send_message(message, from_addr, to_addr)
	smtp.send_message(status_msg, from_addr, status_to_addr)
end
