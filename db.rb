#!/usr/bin/ruby

require 'sqlite3'
require 'progress_bar'
require 'base64'
require 'pry'
require_relative 'lib.rb'

$dbfile = 'test.db'
POSHHEADERS_CREATE_TABLE = %{
CREATE TABLE IF NOT EXISTS POSHheaders(
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	slsman TEXT,
	tikno TEXT,
	piktik TEXT,
	invno TEXT,
	invdte TEXT,
	amtdtl TEXT,
	custno TEXT,
	invdt TEXT,
	piktyp TEXT,
	rprog_c TEXT,
	sig TEXT,
	status TEXT,
	retrieval_error TEXT)
}

TICKET_CONTENTS_CREATE_TABLE = %{
CREATE TABLE IF NOT EXISTS TicketContents (
	id INTEGER PRIMARY KEY AUTOINCREMENT,
	poshheader_id INTEGER,
	contents TEXT NOT NULL)
}

TICKET_CONTENTS_SQL = %{
INSERT INTO TicketContents (contents, poshheader_id) VALUES (?,?)
}

POSHHEADERS_INSERT_RECORD_SQL = %{
INSERT INTO POSHheaders
	(slsman,tikno,piktik,invno,invdte,amtdtl,custno,invdt,piktyp,rprog_c,sig,status,retrieval_error)
VALUES
	(?,?,?,?,?,?,?,?,?,?,?,?,?)
}

$log = Logger.new(STDOUT)
$log.level = Logger::WARN

$config = HBSConfig.new('hbs.config')
if $config.user.nil? || $config.password.nil?
	$log.error "Username and password are required."
	exit 1
end


bot = Automate.new($config.user, Base64.decode64($config.password), $config.ip,
			$config.loc)
bot.login
bot.start_posh
posh_list = bot.request_posh('09')
puts "Found #{posh_list.count} invoices"

stm = nil
begin
	db = SQLite3::Database.open($dbfile)
	db.transaction
	db.execute(POSHHEADERS_CREATE_TABLE)
	db.execute(TICKET_CONTENTS_CREATE_TABLE)
	posh_list.each_with_progress do |poshitem|
		retrieval_error = nil
		begin
			inv_text = bot.get_ticket_preview(poshitem).join("\n")
			
		rescue HBSErrException => e
			puts "HBS Error on invoice ##{poshitem.invno}: #{e.message}"
			retrieval_error = e.message
		end
		stm = db.prepare(POSHHEADERS_INSERT_RECORD_SQL)
		stm.execute(poshitem.slsman, poshitem.tikno, poshitem.piktik,
			poshitem.invno, poshitem.invdte, poshitem.amtdtl,
			poshitem.custno, poshitem.invdt, poshitem.piktyp,
			poshitem.rprog_c, poshitem.sig, poshitem.status,
			retrieval_error)
		stm.close
		stm = nil
		if retrieval_error.nil?
			stm = db.prepare(TICKET_CONTENTS_SQL)
			stm.execute(inv_text, db.last_insert_row_id)
			stm.close
			stm = nil
		end
	end
	db.commit
rescue SQLite3::Exception => e
	puts "SQLite exception"
	puts e
	binding.pry
	db.rollback
ensure
	stm.close if stm
	db.close if db
end

