# Files-backup-app
Client-server, backup-to-disk application.

Simple client-server, backup-to-disk application, which I designed in order to manage some of my files.

The server takes as line arguments an xml file containing current users and a path to the directory the backup
files will be stored.

xml file example: 
	<users>
		<user name="admin" password="test" role="admin"/>
		<user name="mike" password="pass" role="user"/>
	</users>

Clients can be of 2 types: users and admins

Users can:  change their password; 
			list their files;
			delete, rename, upload or download a file;

Admins can: add users;
			list users;
			change the password of a user;
			delete, rename a user;
			impersonate a user (do user operations with that user's signature);
			revert to self (become the original admin again);
