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
the server should send this.
```
{
	"type": "all_users",
	"users": [
		{
			"name": "laptop",
			"status": 1
		},
		{
			"name": "nemesis",
			"status": 0
		}
	]
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
