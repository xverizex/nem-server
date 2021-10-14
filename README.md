# nem server api

1. [Register](#Register)
2. [Login](#Login)
3. [GetList](#GetList)
4. [Handshake](#Handshake)
5. [Message](#Message)
6. [Feed](#Feed)
7. [StatusOnline](#StatusOnline)
8. [HandshakeKey](#HandshakeKey)
9. [HandshakeNotice](#HandshakeNotice)
10. [FileAdd](#FileAdd)
11. [StorageFile](#StorageFile)
12. [GetFile](#GetFile)

## <a name="Register"></a> Register
```
{
	"type": "register",
	"login": "your_login",
	"password": "your_password"
}
```
if a request is valid, then a server answer:
```
{
	"status": "ok"
}
```
if a request is invalid, then a server answer:
```
{
	"status": "false"
}
```
## <a name="Login"></a> Login
```
{
	"type": "login",
	"login": "your_login",
	"password": "your_password"
}
```
if a request is valid, then a server answer:
```
{
	"status": "ok"
}
```
if a request is invalid, then a server answer:
```
{
	"status": "false"
}
```
## <a name="GetList"></a> GetList
```
{
	"type": "get_list"
}
```
the server should send this of each user.
```
{
	"type": "status_online",
	"status", 1,
	"name", "laptop"
}
```
## <a name="Handshake"></a> Handshake
```
{
	"type": "handshake",
	"to_name": "laptop",
	"status": 1,
	"key": "fiopajsefoije0a89fy89yae89fya89sf98asef"
}
```
if a person is unavailable, then is return this:
```
{
	"type": "handshake_answer",
	"status_handshake": 0,
	"to_name": "laptop"
}
```
if a person accepting the request, then the view is this:
```
{
	"type": "handshake_answer",
	"status_handshake": 1,
	"to_name": "laptop"
}
```
## <a name="Message"></a> Message
```
{
	"type": "message",
	"to": "laptop",
	"data": "f8ays8e9fy9a8yh39rh98ahf9ahefhasefasefjj"
}
```
then "laptop" accepts this message:
```
{
	"type": "message",
	"from": "nemesis",
	"data": "f8ays8e9fy9a8yh39rh98ahf9ahefhasefasefjj"
}
```
## <a name="Feed"></a> Feed
```
{
	"type": "feed"
}
```
if the server is store messages then the answer is this:
```
{
	"type": "message",
	"from": "nemesis",
	"to": "laptop",
	"data": "fjaejf89asefa89seyf89asyef98ayhsef"
}
```
a server also can send this data:
## <a name="StatusOnline"></a> StatusOnline
```
{
	"type": "status_online",
	"status", 1,
	"name", "laptop"
}
```
## <a name="HandshakeKey"></a> HandshakeKey
```
{
	"type": "handshake_key",
	"from": "nemesis",
	"key": "fafasjfsa9fasf98sfyha9s8hf98ashf98hasf"
}
```
## <a name="HandshakeNotice"></a> HandshakeNotice
```
{
	"type": "handshake_notice",
	"from": "laptop",
	"status": 1
}
```
## <a name="FileAdd"></a> FileAdd
you should send this.
if is_start is 0 then inserting a new file to the database.
if is_start is 1 then concating new data.
size of JSON packet, not more than 4096 bytes. I recommend a DATA field filling 16 * 80 bytes. CKEY and IVEC are RSA encrypted.
```
{
	"type": "file_add",
	"to": "laptop",
	"filename: "test.txt",
	"ckey": "fa0e0fjeaef...",
	"ivec": "fase9fjasef9aef...",
	"data": "fajse90fjaefafeahefahef",
	"is_start": 0
}
```
## <a name="StorageFile"></a> StorageFile
you should send this data.
```
{
	"type": "storage_files",
	"from": "laptop"
}
```
the answer will be it.
```
{
	"type": "storage_files",
	"from": "pcpc",
	"filename": "test.png",
	"ckey": "fa9fe09asefsjef...",
	"ivec": "faefj0a9ejfae..."
}
```
## <a name="GetFile"></a> GetFile
send this.
```
{
	"type": "get_file",
	"from": "laptop",
	"filename": "test.png"
}
```
the answer will be it.
```
{
	"type": "getting_file",
	"from": "laptop",
	"filename": "test.png",
	"ckey": "faioefasfjeiaef0...",
	"ivec": "faiojasefajsefj...",
	"data": "aiofjaiojsefjioasoie",
	"pos": 0,
	"size": 13720
}
```
