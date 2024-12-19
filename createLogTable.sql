-- Script to create the table `stove_log`

CREATE TABLE stove_log (
    creation_date DATETIME NOT NULL  DEFAULT CURRENT_TIMESTAMP  PRIMARY KEY,
    datagram BLOB(255) NOT NULL
);

