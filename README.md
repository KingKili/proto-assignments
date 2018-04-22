# protocol-assignment-1

### ToDo:
* Add a way to indicate for wich file the package is in the  ```File-Creation``` and ```File-Transfer``` message

### Important:
* UDP MTU

### Changelog:
* 20.04.2018 [Fabian] Initial commit
* 22.04.2018 [Fabian] Protocol

## Protocol:

### General field descriptions:
Type [4 Bit]:<br/>
	0000 => Client-Hello-Handshake<br/>
	0001 => Server-Hello-Handshake<br/>
	0010 => File-Creation<br/>
	0011 => File-Transfer<br/>
	0100 => Server-ACK<br/>

Client ID [48 Bit]:<br/>
	An unique client id generated by the server on first contact<br/>
	e.g. static filed with an int that gets incremented for each connected client

Checksum [16 Bit]:<br/>
	Like TCP

Sequence Number [32 bit]:<br/>
	Like TCP

### Client-Hello-Handshake:
```
0      4      20         36
+------+------+----------+
| Type | Port | Checksum |
+------+------+----------+
```

Port [16 Bit]:<br/>
	The port on which the client listens to server messages

### Server-Hello-Handshake:
```
0      4       8           56            72         88
+------+-------+-----------+-------------+----------+
| Type | Flags | Client ID | Upload-port | Checksum |
+------+-------+-----------+-------------+----------+
```
Upload-port [16 Bit]:<br/>
	The Port where the client should send all following messages to

Flags [4 Bit]:
```
0000
||||
|||+-> Client accepted
||+--> Too many clients - connection revoked
|+---> *UNUSED*
+----> *UNUSED*
```

### File-Creation:
```
0      4           52                84          86                 118
+------+-----------+-----------------+-----------+------------------+-----------+
| Type | Client ID | Sequence Number | File Type | File Name Length | File Name |
+------+-----------+-----------------+-----------+------------------+-----------+
+-------------------------+------------------+----------+
| Relat. File Path Length | Relat. File Path | Checksum |
+-------------------------+------------------+----------+
```

File Type [2 Bit]:<br/>
	00 => Folder<br/>
	01 => File<br/>
	10 => *UNUSED*<br/>
	11 => *UNUSED*<br/>

File Name Length [x Bit] **Don't forget about the MTU**

File Name Length [defined in "File Name Length" in Bit]

Relat. File Path Length [x Bit] **Don't forget about the MTU**

Relat. File Path [defined in "Relat. File Path Length" in Bit]

### File-Transfer:
```
0      4           52                84      88
+------+-----------+-----------------+-------+----------------+---------+----------+
| Type | Client ID | Sequence Number | Flags | Content Length | Content | Checksum |
+------+-----------+-----------------+-------+----------------+---------+----------+
```

Flags [4 Bit]:
```
0000
||||
|||+-> First package for the given file
||+--> File content
|+---> *UNUSED*
+----> Last package for the file
```

Content Length [x Bit] **Don't forget about the MTU**

Content [defined in "Content Length" in Bit] **Don't forget about the MTU**

### Server-ACK:
```
0      4           52                    84
+------+-----------+---------------------+
| Type | Client ID | ACK Sequence Number |
+------+-----------+---------------------+
```

ACK Sequence Number [like Sequence Number]:<br/>
	The acknowledged sequence number

### Transfer-Ended:
```
0      4       8           56         72
+------+-------+-----------+----------+
| Type | Flags | Client ID | Checksum |
+------+-------+-----------+----------+
```

Flags [4 Bit]:
```
0000
||||
|||+-> Transfer finished
||+--> Canceled by user
|+---> Error
+----> *UNUSED*
```

## Process example:

```
      Client				  Server
	|	Client-Hello-Handshake	    |
	| --------------------------------> | The clients starts the connection on the default port
	|				    | and tells the server the port on which he listens for answers
	|	Server-Hello-Handshake      |
	| <-------------------------------- | The server responds with a client ID and a port where the
	|				    | server is listening for incomming transfer messages
	|	File-Creation		    |
	| --------------------------------> | The client sends this message to inform the server about
	|				    | the new file that will be transfered
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> | The client starts sending the file
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	File-Transfer		    |
	| --------------------------------> |
	|				    |
	|	Server-ACK  		    |
	| <-------------------------------- | The server sends an ACK message for each message
	|				    | it received from the client
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	Server-ACK  		    |
	| <-------------------------------- |
	|				    |
	|	Transfer-Ended		    |
	| --------------------------------> | The client tells the server that the transfer finished
```
