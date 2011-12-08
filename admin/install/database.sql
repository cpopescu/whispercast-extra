drop database if exists `@@@DB_NAME@@@`;
create database `@@@DB_NAME@@@`;
grant all on `@@@DB_NAME@@@`.* to '@@@DB_USER@@@'@'@@@DB_HOST@@@' identified by '@@@DB_PASSWORD@@@';

use `@@@DB_NAME@@@`;

-- ###############################################################

-- phpMyAdmin SQL Dump
-- version 2.11.3deb1ubuntu1.3
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Apr 30, 2010 at 03:38 AM
-- Server version: 5.0.51
-- PHP Version: 5.2.4-2ubuntu5.10

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

--
-- Database: `@@@DB_NAME@@@`
--

-- --------------------------------------------------------

--
-- Table structure for table `acl`
--

CREATE TABLE IF NOT EXISTS `acl` (
  `server_id` int(11) NOT NULL default '0',
  `user_id` int(11) NOT NULL,
  `resource` varchar(255) collate utf8_unicode_ci NOT NULL,
  `resource_id` int(11) NOT NULL default '0',
  `action` set('add','set','get','delete','operate','osd') collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`server_id`,`user_id`,`resource`,`resource_id`,`action`),
  KEY `user_id` (`user_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `aliases`
--

CREATE TABLE IF NOT EXISTS `aliases` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  `export_as` varchar(255) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `files`
--

CREATE TABLE IF NOT EXISTS `files` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  `internal_id` bigint(20) NOT NULL default '-1',
  `state` enum('new','uploaded','waiting','transferring','transferred','transcoding','failed') collate utf8_unicode_ci default 'new',
  `key` char(32) collate utf8_unicode_ci NOT NULL,
  `processed` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `filesTranscoded`
--

CREATE TABLE IF NOT EXISTS `filesTranscoded` (
  `id` int(11) NOT NULL auto_increment,
  `file_id` int(11) NOT NULL,
  `width` int(11) default NULL,
  `height` int(11) default NULL,
  `framerate_n` int(11) default NULL,
  `framerate_d` int(11) default NULL,
  `bitrate` int(11) default NULL,
  `duration` bigint(20) default NULL,
  `file` varchar(255) collate utf8_unicode_ci NOT NULL,
  `available` tinyint(1) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `file_id` (`file_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `aggregators`
--

CREATE TABLE IF NOT EXISTS `aggregators` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `playlists`
--

CREATE TABLE IF NOT EXISTS `playlists` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  `loop` tinyint(1) NOT NULL default '0',
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `programs`
--

CREATE TABLE IF NOT EXISTS `programs` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `ranges`
--

CREATE TABLE IF NOT EXISTS `ranges` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  `source_id` int(11) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `source_id` (`source_id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `remotes`
--

CREATE TABLE IF NOT EXISTS `remotes` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `servers`
--

CREATE TABLE IF NOT EXISTS `servers` (
  `id` int(11) NOT NULL auto_increment,
  `version` int(11) unsigned NOT NULL,
  `name` varchar(255) collate utf8_unicode_ci NOT NULL,
  `host` varchar(255) collate utf8_unicode_ci NOT NULL,
  `http_port` smallint(5) unsigned NOT NULL,
  `rtmp_port` smallint(5) unsigned NOT NULL,
  `export_host` varchar(255) collate utf8_unicode_ci NOT NULL,
  `export_base` varchar(255) collate utf8_unicode_ci NOT NULL,
  `disks` tinyint(4) NOT NULL,
  `preview_user` varchar(32) collate utf8_unicode_ci NOT NULL,
  `preview_password` varchar(32) collate utf8_unicode_ci NOT NULL,
  `admin_port` smallint(5) unsigned NOT NULL,
  `admin_user` varchar(255) collate utf8_unicode_ci NOT NULL,
  `admin_password` varchar(255) collate utf8_unicode_ci NOT NULL,
  `transcoder_port` smallint(5) unsigned NOT NULL,
  `transcoder_user` varchar(255) collate utf8_unicode_ci NOT NULL,
  `transcoder_password` varchar(255) collate utf8_unicode_ci NOT NULL,
  `runner_port` smallint(5) unsigned NOT NULL,
  `runner_user` varchar(255) collate utf8_unicode_ci NOT NULL,
  `runner_password` varchar(255) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Triggers `servers`
--
DROP TRIGGER IF EXISTS `@@@DB_NAME@@@`.`serversAD`;
DELIMITER //
CREATE TRIGGER `@@@DB_NAME@@@`.`serversAD` AFTER DELETE ON `@@@DB_NAME@@@`.`servers`
 FOR EACH ROW BEGIN

  DELETE FROM acl WHERE acl.server_id = OLD.id;
  DELETE FROM acl WHERE acl.resource_id = OLD.id AND resource = 'servers';

END
//
DELIMITER ;

-- --------------------------------------------------------

--
-- Table structure for table `sources`
--

CREATE TABLE IF NOT EXISTS `sources` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  `server_id` int(11) NOT NULL,
  `protocol` enum('http','rtmp') collate utf8_unicode_ci NOT NULL,
  `source` varchar(255) collate utf8_unicode_ci NOT NULL,
  `password` varchar(255) collate utf8_unicode_ci default NULL,
  `saved` tinyint(1) NOT NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `server_id_protocol_source` (`server_id`,`protocol`,`source`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `streams`
--

CREATE TABLE IF NOT EXISTS `streams` (
  `id` int(11) NOT NULL auto_increment,
  `server_id` int(11) NOT NULL,
  `category_id` int(11) default NULL,
  `type` enum('sources','switches','files','aliases','playlists','programs','ranges','remotes','aggregators') collate utf8_unicode_ci NOT NULL,
  `name` varchar(255) collate utf8_unicode_ci NOT NULL,
  `description` text collate utf8_unicode_ci,
  `tags` text collate utf8_unicode_ci,
  `duration` bigint(20) default NULL,
  `path` varchar(255) collate utf8_unicode_ci default NULL,
  `export` varchar(255) collate utf8_unicode_ci default NULL,
  `public` tinyint(1) NOT NULL default '0',
  `usable` tinyint(1) NOT NULL default '1',
  PRIMARY KEY  (`id`),
  KEY `category_id` (`category_id`),
  KEY `server_id` (`server_id`),
  KEY `name` (`name`),
  KEY `duration` (`duration`),
  KEY `public` (`public`),
  KEY `usable` (`usable`),
  KEY `type` (`type`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Triggers `streams`
--
DROP TRIGGER IF EXISTS `@@@DB_NAME@@@`.`streamsAI`;
DELIMITER //
CREATE TRIGGER `@@@DB_NAME@@@`.`streamsAI` AFTER INSERT ON `@@@DB_NAME@@@`.`streams`
 FOR EACH ROW BEGIN

    INSERT INTO streamsMeta SET id = NEW.id, name = NEW.name, description = NEW.description, tags = NEW.tags;

  END
//
DELIMITER ;
DROP TRIGGER IF EXISTS `@@@DB_NAME@@@`.`streamsAU`;
DELIMITER //
CREATE TRIGGER `@@@DB_NAME@@@`.`streamsAU` AFTER UPDATE ON `@@@DB_NAME@@@`.`streams`
 FOR EACH ROW BEGIN

    UPDATE streamsMeta SET name = NEW.name, description = NEW.description, tags = NEW.tags WHERE id = NEW.id;

  END
//
DELIMITER ;
DROP TRIGGER IF EXISTS `@@@DB_NAME@@@`.`streamsAD`;
DELIMITER //
CREATE TRIGGER `@@@DB_NAME@@@`.`streamsAD` AFTER DELETE ON `@@@DB_NAME@@@`.`streams`
 FOR EACH ROW BEGIN

    DELETE FROM streamsMeta WHERE id = OLD.id;

  END
//
DELIMITER ;

-- --------------------------------------------------------

--
-- Table structure for table `streamsCategories`
--

CREATE TABLE IF NOT EXISTS `streamsCategories` (
  `id` int(11) NOT NULL auto_increment,
  `server_id` int(11) NOT NULL,
  `name` varchar(255) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `server_id` (`server_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `streamsMeta`
--

CREATE TABLE IF NOT EXISTS `streamsMeta` (
  `id` int(11) NOT NULL,
  `name` varchar(255) collate utf8_unicode_ci NOT NULL,
  `description` text collate utf8_unicode_ci,
  `tags` text collate utf8_unicode_ci,
  KEY `id` (`id`),
  FULLTEXT KEY `name_description_tags` (`name`,`description`,`tags`),
  FULLTEXT KEY `description` (`description`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `switches`
--

CREATE TABLE IF NOT EXISTS `switches` (
  `id` int(11) NOT NULL auto_increment,
  `stream_id` int(11) NOT NULL,
  PRIMARY KEY  (`id`),
  KEY `stream_id` (`stream_id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

-- --------------------------------------------------------

--
-- Table structure for table `users`
--

CREATE TABLE IF NOT EXISTS `users` (
  `id` int(11) NOT NULL auto_increment,
  `user` varchar(32) collate utf8_unicode_ci NOT NULL,
  `password` varchar(32) collate utf8_unicode_ci NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=InnoDB  DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci AUTO_INCREMENT=1 ;

--
-- Constraints for dumped tables
--

--
-- Constraints for table `acl`
--
ALTER TABLE `acl`
  ADD CONSTRAINT `acl_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `users` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `aliases`
--
ALTER TABLE `aliases`
  ADD CONSTRAINT `aliases_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `files`
--
ALTER TABLE `files`
  ADD CONSTRAINT `files_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `filesTranscoded`
--
ALTER TABLE `filesTranscoded`
  ADD CONSTRAINT `filesTranscoded_ibfk_1` FOREIGN KEY (`file_id`) REFERENCES `files` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `aggregators`
--
ALTER TABLE `aggregators`
  ADD CONSTRAINT `aggregators_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `playlists`
--
ALTER TABLE `playlists`
  ADD CONSTRAINT `playlists_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `programs`
--
ALTER TABLE `programs`
  ADD CONSTRAINT `programs_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `ranges`
--
ALTER TABLE `ranges`
  ADD CONSTRAINT `ranges_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `sources`
--
ALTER TABLE `sources`
  ADD CONSTRAINT `sources_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `streams`
--
ALTER TABLE `streams`
  ADD CONSTRAINT `streams_ibfk_1` FOREIGN KEY (`category_id`) REFERENCES `streamsCategories` (`id`) ON DELETE SET NULL ON UPDATE CASCADE;

--
-- Constraints for table `streamsCategories`
--
ALTER TABLE `streamsCategories`
  ADD CONSTRAINT `streamsCategories_ibfk_1` FOREIGN KEY (`server_id`) REFERENCES `servers` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

--
-- Constraints for table `switches`
--
ALTER TABLE `switches`
  ADD CONSTRAINT `switches_ibfk_1` FOREIGN KEY (`stream_id`) REFERENCES `streams` (`id`) ON DELETE CASCADE ON UPDATE CASCADE;

-- ###############################################################

insert into users values (NULL, 'admin', md5('@@@ADMIN_PASSWORD@@@'));
insert into acl select 0, id, 'servers', 0, 'add' from users where user='admin';
