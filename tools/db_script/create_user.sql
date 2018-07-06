create database if not exists #DB# default character set = utf8;
use #DB#;

create table if not exists userdata_#TID# (
    uid_key bigint unsigned not null comment '主键key + uid',
    data blob not null comment 'pb 字节序',
    version int unsigned not null comment '数据版本号',
	primary key(uid_key)	
) engine = InnoDB, charset = utf8;
