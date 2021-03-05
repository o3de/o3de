CREATE TABLE `categories` (
  `category_id` INTEGER PRIMARY KEY,
  `category` varchar(255) UNIQUE NOT NULL,
  `order_id` int(10) NOT NULL,
  PRIMARY KEY  (`category_id`)
);

CREATE TABLE IF NOT EXISTS `tags` (
  `id` INTEGER PRIMARY KEY,
  `tag` varchar(64) NOT NULL,
  `category_id` INTEGER NOT NULL,
  FOREIGN KEY (`category_id`) REFERENCES `categories` (`category_id`) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS `projects` (
  `id` INTEGER PRIMARY KEY,
  `name` varchar(32) UNIQUE NOT NULL
);


CREATE TABLE IF NOT EXISTS `asset_inventory` (
  `id` INTEGER PRIMARY KEY,
  `project_id` INTEGER NOT NULL,
  `relpath` varchar(255) NOT NULL,
  `description` varchar(255),
  FOREIGN KEY (`project_id`) REFERENCES `projects` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE IF NOT EXISTS `asset_tags` (
  `asset_id` INTEGER NOT NULL,
  `tag_id` INTEGER NOT NULL,
  PRIMARY KEY (`asset_id`,`tag_id`),
  FOREIGN KEY (`asset_id`) REFERENCES `asset_inventory` (`id`) ON DELETE CASCADE ON UPDATE CASCADE,
  FOREIGN KEY (`tag_id`) REFERENCES `tags` (`id`) ON DELETE CASCADE ON UPDATE CASCADE
);
