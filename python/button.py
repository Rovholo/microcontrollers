import socket
import time
from zeroconf import ServiceInfo, ServiceBrowser, Zeroconf

class ServiceListener:
    def remove_service(self, zeroconf, type, name):
        print(f"Service {name} removed")

    def add_service(self, zeroconf, type, name):
        info = zeroconf.get_service_info(type, name)
        if info:
            # info.parsed_addresses contains the resolved IP addresses
            print(f"Service {name} added, address: {', '.join(info.parsed_addresses)}")
            print(f"  Port: {info.port}")
            # You can stop the script or store the address for later use here

# Example of how to use it
if __name__ == '__main__':
    # 'Zeroconf' instance manages mDNS operations in the background
    zeroconf = Zeroconf()
    listener = ServiceListener()
    
    # Browse for services of a specific type (e.g., http, ssh, etc.)
    # The standard way to resolve a host name is to look for a service it offers
    print("\nBrowsing for _http._tcp.local. services...")
    browser = ServiceBrowser(zeroconf, "_http._tcp.local.", listener)
    
    try:
        # Keep the script running to listen for mDNS responses
        input("Press enter to exit...\n")
    finally:
        # Clean up resources
        zeroconf.close()

