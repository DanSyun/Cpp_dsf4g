create database if not exists #DB# default character set = utf8;
use #DB#;

create table if not exists userdata_#TID# (
    uid_key bigint unsigned not null comment '����key + uid',
    data blob not null comment 'pb �ֽ���',
    version int unsigned not null comment '���ݰ汾��',
	primary key(uid_key)	
) engine = InnoDB, charset = utf8;
