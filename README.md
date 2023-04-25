README.md:

# chatgpt_cli

OpenAI-powered command-line interface for chatbot conversations.

This software uses [OpenAI](https://openai.com/) to generate chatbot conversations.
To use this software, you will need to sign up for an API key from OpenAI. Please
visit the OpenAI website to acquire an API key.

By using this software, you agree to the terms set forth in the [LICENSE](LICENSE) and
[LEGAL.txt](LEGAL.txt) documents. Please read these documents carefully before using
the software.

## License

This software is licensed under the [MIT] license. Please refer to the
[LICENSE](LICENSE) file for more information.

## Legal

The software provided by Fudmottin is strictly experimental. The user assumes
all liability for its use. Fudmottin shall have no liability to the user for any
damages whatsoever arising out of or related to the use of the software.

By using this software, you agree to indemnify and hold harmless Fudmottin, its
affiliates, directors, officers, employees, agents, and representatives from any and
all claims, losses, liabilities, damages, costs, and expenses (including reasonable
attorneys' fees) arising out of or related to your use of the software.

Please note that this software is provided on an "as is" basis without any warranties
or representations of any kind, express or implied.

## Building

mkdir build && cd build
cmake ..
make

Hopefully that will work. I've been using macOS for development. There are a couple brew
packages you need. One is cpr. This does the HTTPS request handling. The other is
nlohmann-json. You'll never guess what it's used for.

It is possible that you may have to modify the CMakeLists.txt file. Brew often uses
/opt/homebrew blah blah blah. If you're on Linux apt-get is your friend. RPM distros
should work also. I haven't tested on Linux yet. All I have is an unplugged Raspbery
Pi somewhere.

As this is experimental, there will be changes (unless I lose interest). Better
command processing is desired. You type quit to quit. Or you can do CTRL-C. Integrating
other models is also on the TODO list.

Windows users are on their own. The code is pretty standard C++ stuff.
