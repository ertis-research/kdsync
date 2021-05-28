#include <cppkafka/cppkafka.h>
#include <derecho/core/derecho.hpp>
#include <derecho/mutils-serialization/SerializationSupport.hpp>

using namespace std;
using namespace cppkafka;
using namespace mutils;

class SerializableMessage : /*Message,*/ public ByteRepresentable {
    //vector<tuple<string, string>> message_list;
    string message;
    //unsigned char* message;

    public:

        string get_message(){
            return message;
        }

        void set_message(const string& message){
            this->message = message;
        }

        SerializableMessage(const string& message="<empty>") : message(message) {}

        DEFAULT_SERIALIZATION_SUPPORT(SerializableMessage, message);
        REGISTER_RPC_FUNCTIONS(SerializableMessage, get_message, set_message);

};