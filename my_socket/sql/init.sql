CREATE DATABASE IF NOT EXISTS netdisk DEFAULT CHARACTER SET utf8mb4;
USE netdisk;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE,
    password VARCHAR(255) NOT NULL,
    is_deleted TINYINT(1) NOT NULL DEFAULT 0,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

INSERT INTO users (username, password, is_deleted)
VALUES ('xkx', 'pig', 0)
ON DUPLICATE KEY UPDATE
    password = VALUES(password),
    is_deleted = 0;
