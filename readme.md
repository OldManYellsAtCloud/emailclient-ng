This a simple, read only email client. It is developed for my own use, most on the PinePhone - I couldn't 
find any dead simple clients with dead simple GUI that would also use only a small amount of resources, so I wrote this.

Most probably you don't want to use this, it is very tailored to my needs.

But a few words:
- UI is in QML/Qt6
- mails are pulled using libCurl requiests, through IMAP
- already fetched mails are stored an SQLite3 database
- mails are fetched periodically from a preconfigured set of folders
- mails are displayed with a WebView component

It has been only used with GMail.
