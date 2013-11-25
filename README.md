geb1150_proxy
=============

Windows+Linux libusb proxy for Franklin GEB1150 ebook readers.

This is an old libusb driver that I wrote to upload ebooks to my GEB1150
Franklin ebook reader in Linux.  I used it in the following manner:

* Start eBook Librarian in wine: http://www.breeno.org/_eBook/ (possibly sudo)
* Start gebusb1150_proxy (possibly sudo)
* Point eBook Librarian to localhost:8079 (PROXY_PORT in the source)
* Connect your ebook reader
* List and download ebooks using the ebook reader interface

NOTES
-----
* This compiles fine in Ubuntu x64 with libusb-dev installed
* You may need to modify your Makefile to point to the correct libusb.a location.

AUTHOR
------
Eugene Brevdo (original code circa 2006)

