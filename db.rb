#!/usr/bin/ruby

require 'sqlite3'
$dbfile = 'test.db'
$table = 'CREATE TABLE IF NOT EXISTS POSHheaders(
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
	retrieval_error INTEGER NOT NULL
)'
begin
	db = SQLite3::Database.open $dbfile
	db.execute $table
	db.execute ""
rescue SQLite3::Exception => e
	puts "Exception occurred"
	puts e
ensure
	db.close if db
end
