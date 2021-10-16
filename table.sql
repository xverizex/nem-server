drop table if exists online;
drop table if exists waitness;
drop table if exists handshake;
drop table if exists storage;

create table if not exists acc (
	id int not null primary key auto_increment,
	name char(16),
	password char(16)
	);

create table if not exists online (
	id int not null primary key auto_increment,
	id_person int not null,
	name char(16) not null,
	ssl_ptr long not null
	);

create table if not exists waitness (
	id int not null primary key auto_increment,
	name_to char(16) not null,
	name_from char(16) not null,
	data TEXT not null
);

create table if not exists handshake (
	id int not null primary key auto_increment,
	ssl_ptr long not null,
	to_name char(16) not null,
	key_pem TEXT not null
);

create table if not exists storage (
	id int not null primary key auto_increment,
	ckey TEXT not null,
	ivec TEXT not null,
	name_from char(16) not null,
	name_to char(16) not null,
	filename char(255) not null,
	data_path char(255) not null
	)
