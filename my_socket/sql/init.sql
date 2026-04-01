CREATE DATABASE IF NOT EXISTS netdisk DEFAULT CHARACTER SET utf8mb4;
USE netdisk;

DROP TABLE IF EXISTS files;
DROP TABLE IF EXISTS users;

CREATE TABLE IF NOT EXISTS users (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(64) NOT NULL UNIQUE,
    password_hash VARCHAR(255) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS files (
    id BIGINT PRIMARY KEY AUTO_INCREMENT,
    user_id BIGINT NOT NULL,
    filename VARCHAR(255) NOT NULL,
    filepath VARCHAR(512) NOT NULL,
    filesize BIGINT NOT NULL DEFAULT 0,
    filetype VARCHAR(64) NOT NULL DEFAULT '',
    sha256 CHAR(64) NOT NULL DEFAULT '',
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    INDEX idx_files_user_id(user_id),
    INDEX idx_files_filename(filename),
    CONSTRAINT fk_files_user FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

INSERT INTO users (username, password_hash)
VALUES ('xkx', 'pig'),
         ('yyq', 'dog')
ON DUPLICATE KEY UPDATE
    password_hash = VALUES(password_hash);

SET @seed_user_id = (SELECT id FROM users WHERE username = 'xkx' LIMIT 1);
INSERT INTO files (id, user_id, filename, filepath, filesize, filetype, sha256)
VALUES
    (1, @seed_user_id, '(1).png', '../files/(1).png', 1837239, 'png', '615f72e54760ff6a64cf6cb4457005723170dd20cf24304d53796f420b1b74c5'),
    (2, @seed_user_id, '(17).png', '../files/(17).png', 3632595, 'png', 'e8073cb42bc57048b86ea80eaf5c48334424ddb9f19527d9bced31832b0316bd'),
    (3, @seed_user_id, '117168586377184961.png', '../files/117168586377184961.png', 534188, 'png', '6cdc2aba976677ddb518fd16a4f7f6b8d86fb7efaa701a5005c01616217ddd0b'),
    (4, @seed_user_id, '20240506192717.jpg', '../files/20240506192717.jpg', 43160, 'jpg', '27df1a9fa4b4fd3a57731ea3904aaba2dbb0a260022df7d2e121f812ac080757'),
    (5, @seed_user_id, 'khl20231227233320353.png', '../files/khl20231227233320353.png', 2779971, 'png', 'd821282a4b12d2adc69ff5dff8b5260686da416740e426ab11ed2a3e9b7fdf2f'),
    (6, @seed_user_id, 'kitty.jpg', '../files/kitty.jpg', 97433, 'jpg', 'e00a920602e202aaae2２３６db908a3１４２b２１１a６９c８８２d３d５５d４８ba３５７０f４cd３１d'),
    (7, @seed_user_id, 'select.html', '../files/select.html', 1906, 'html', '8c1de7d0553d827083ca611bf14424ffb3e2554ab399b87694570f006357459e'),
    (8, @seed_user_id, 'xkx.jpg', '../files/xkx.jpg', 67675, 'jpg', 'e4dcccda2f1e8af43a3a80c7a2d36af9283b2673d837d051b3dd250490fdd7ee')
ON DUPLICATE KEY UPDATE
    user_id = VALUES(user_id),
    filename = VALUES(filename),
    filepath = VALUES(filepath),
    filesize = VALUES(filesize),
    filetype = VALUES(filetype),
    sha256 = VALUES(sha256),
    updated_at = CURRENT_TIMESTAMP;
