# radosgw-admin
implements the parsing of the following radosgw-admin commands:

radosgw-admin user create --uid=<uid> --display-name=<name> --email=<email>
  
radosgw-admin user delete --uid=<uid>
  
radosgw-admin user info --uid=<uid>

When run these commands should output that they have been successfully run and output the arguments passed to them.

ex:

$ radosgw-admin user create --uid=200 --display-name=foo --email=foo@gmail.com
user created with uid 200 display-name foo and email foo@gmail.com

radosgw-admin --help - will display all the options copied for the real radosgw-admin command help info in Ceph.

Any invalid command will give an error and display the help text

For assembly we use cmake.

This implementation greatly simplifies the addition of a new command, reduces the number of code repetitions, improves the code structure and we added the use of boost::program_options
