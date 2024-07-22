from ldap3 import Server, Connection, ALL, NTLM, SIMPLE, SUBTREE

# Define the server and connection parameters
ldap_server_url = 'localhost'
ldap_bind_dn = 'cn=admin,dc=buergerProxmox'  # Replace with your Bind DN
ldap_bind_password = 'adminpw'  # Replace with your password
search_base = 'dc=buergerProxmox'  # Replace with your search base
search_filter = '(objectClass=person)'  # Adjust the filter as needed

# Create an LDAP server object
server = Server(ldap_server_url) #, get_info=ALL

# Create a connection object
connection = Connection(server, user=ldap_bind_dn, password=ldap_bind_password, auto_bind=True)

# Check if the connection is established
if not connection.bind():
    print('Error in binding to the server')
else:
    print('Successfully bound to the server')

    # Perform a search
    connection.search(search_base, search_filter, search_scope=SUBTREE, attributes=['cn', 'sn', 'mail'])

    # Print the results
    for entry in connection.entries:
        print(entry)

    # Unbind the connection
    connection.unbind()