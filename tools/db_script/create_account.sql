create database if not exists #DB# default character set = utf8;
use #DB#;

create table if not exists account_#TID# (
    account_type int unsigned not null comment '账号类型',
    account_name varchar(64) not null comment '账号名',
    password varchar(64) default '' comment '密码',
    uid bigint unsigned not null comment 'uid',
	primary key(account_type, account_name)	
) engine = InnoDB, charset = utf8;

create table if not exists uid_index (
    uid bigint unsigned not null comment '当前最大uid',
    primary key (uid)
) engine = InnoDB, charset = utf8;