SET @db_user='happhub';
SET @db_user_host='localhost';
SET @db_user_password='@@@password@@@';
SET CHARACTER SET utf8;

DROP DATABASE IF EXISTS `happhub`;
CREATE DATABASE `happhub`;

SET @sql = CONCAT("GRANT USAGE ON *.* TO ",QUOTE(@db_user),"@",QUOTE(@db_user_host));
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = CONCAT("DROP USER ",QUOTE(@db_user),"@",QUOTE(@db_user_host));
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

SET @sql = CONCAT("CREATE USER ",QUOTE(@db_user),"@",QUOTE(@db_user_host)," IDENTIFIED BY ",QUOTE(@db_user_password));
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;
SET @sql = CONCAT("GRANT ALL PRIVILEGES ON `happhub`.* TO ",QUOTE(@db_user),"@",QUOTE(@db_user_host));
PREPARE stmt FROM @sql; EXECUTE stmt; DEALLOCATE PREPARE stmt;

USE `happhub`;
CREATE TABLE `users` (`id` int NOT NULL AUTO_INCREMENT, `email` varchar(255) NOT NULL, `password` char(64) DEFAULT NULL, PRIMARY KEY (`id`), UNIQUE (`email`)) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `accounts` (`id` int NOT NULL AUTO_INCREMENT, `name` varchar(255) NOT NULL, PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `roles` (`accounts_id` int NOT NULL, `users_id` int NOT NULL, `role` enum('owner','attached') NOT NULL, PRIMARY KEY (`users_id`, `accounts_id`), CONSTRAINT `fk_accounts_users_accounts` FOREIGN KEY (`accounts_id`) REFERENCES `accounts` (`id`) ON DELETE CASCADE ON UPDATE CASCADE, CONSTRAINT `fk_accounts_users_users` FOREIGN KEY (`users_id`) REFERENCES `users` (`id`) ON DELETE CASCADE ON UPDATE CASCADE) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `servers` (`id` int NOT NULL AUTO_INCREMENT, `name` varchar(255) NOT NULL, `protocol` char(8) NOT NULL, `host` varchar(255) NOT NULL, `port` int NOT NULL, `path` varchar(255) NOT NULL, PRIMARY KEY (`id`)) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `sessions` (`id` int NOT NULL AUTO_INCREMENT, `accounts_id` int NOT NULL, `servers_id` int NOT NULL, PRIMARY KEY (`id`), `name` varchar(255) NOT NULL, CONSTRAINT `fk_sessions_accounts` FOREIGN KEY (`accounts_id`) REFERENCES `accounts` (`id`) ON DELETE CASCADE ON UPDATE CASCADE, CONSTRAINT `fk_sessions_servers` FOREIGN KEY (`servers_id`) REFERENCES `servers` (`id`) ON DELETE CASCADE ON UPDATE CASCADE) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `cameras` (`id` int NOT NULL AUTO_INCREMENT, `sessions_id` int NOT NULL, PRIMARY KEY (`id`), `name` varchar(255) NOT NULL, CONSTRAINT `fk_cameras_sessions` FOREIGN KEY (`sessions_id`) REFERENCES `sessions` (`id`) ON DELETE CASCADE ON UPDATE CASCADE) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE TABLE `signups` (`hash` char(64) NOT NULL, `email` varchar(255) NOT NULL, `accounts_id` int NOT NULL, PRIMARY KEY (`hash`)) ENGINE=InnoDB DEFAULT CHARSET utf8;
CREATE VIEW `auth_users` AS SELECT `users`.`email` AS `email`,`users`.`password` AS `password` FROM `users`;
CREATE VIEW `auth_roles` AS SELECT `users`.`email` AS `email`,`roles`.`role` AS `role` FROM `roles` INNER JOIN `users` ON `users`.`id` = `roles`.`users_id` INNER JOIN `accounts` ON `accounts`.`id` = `roles`.`accounts_id` WHERE `accounts`.`id` IS NOT NULL AND `users`.`id` IS NOT NULL;

insert into servers values(null, "Default", "http", "95.64.125.44", 8080, "/");
