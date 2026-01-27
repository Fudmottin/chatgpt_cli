# clanker

Some serious wishful thinking going on here.


Clanker is intended to be a shell like TUI for LLMs. The special feature is that
clanker will allow chaining models together like they are any other program. It
also provides a uniform front end for different models from OpenAI, Anthropic,
etc. In short, it is model agnostic.


Some models don't output text. Instead they will output images, video, audo,
etc. It will be up to the user to know what to expect, but perhaps clanker will
be able to help there as well. The assumed interface to an LLM is an HTTPS POST
request with a JSON body. The asumed rsponse includes a standard response code
and JSON body. Where MIME types are used, clanker should be able to properly
deal with the response in the appropriate way, ie converting base64 encoded data
into binary form for directing to a file or another process like ffmpeg,
ImageMagick, etc.

