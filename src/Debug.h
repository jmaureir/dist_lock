#ifndef __C_DEBUG__
#define __C_DEBUG__

#include <iostream>
#include <sstream>
#include <map>
#include <typeindex>
#include <typeinfo>
#include <functional>
#include <cxxabi.h>


class Debug {
	private:
		bool enabled;

        static Debug* d;

        const std::string demangle(const char* name) {
            int status = -4;
            char* res = abi::__cxa_demangle(name, NULL, NULL, &status);
            const char* const demangled_name = (status==0)?res:name;
            std::string ret_val(demangled_name);
            free(res);
            return ret_val;
        }

	public:

        enum LogType {
            NONE      = 0,
            INFO      = 1,
            WARN      = 2,
            CRITICAL  = 4,
            DEBUG1    = 8, 
            DEBUG2    = 16,
            DEBUG3    = 32,
            ALL       = 255
        };

        class CoutBase {
            protected:
                std::ostringstream buffer;
        		uint64_t id;
                bool enabled;
                unsigned int log_mask;

            public:
                CoutBase(uint64_t id, bool e = false) {
                    this->id        = id;
                    this->enabled   = e;
                }
                virtual ~CoutBase() {
                    std::cout << buffer.str();
                    std::cout.flush();
                }

                void setLogMask(unsigned int l) {
                    this->log_mask = l;
                }

                void setDebugId(uint64_t id) {
                    this->id = id;
                }

                void setEnabled(bool flag) {
                    this->enabled = flag;
                }

                bool isEnable() {
                    return this->enabled;
                }

                template <typename T> Debug::CoutBase& operator<< (const T& o) {
                    if (this->log_mask = ALL) {
                        this->enabled = true;
                    }
                    if (this->enabled) {
                        if (&o != NULL) {
                            buffer << o;
                        } else {
                            buffer << "NULL";
                        }
                    }
                    return *this;
                }

                // function that takes a custom stream, and returns it
                typedef Debug::CoutBase& (*MyStreamManipulator)(Debug::CoutBase&);

                // function with a custom signature	
                Debug::CoutBase& operator<<(MyStreamManipulator manip) {
                    // call the function, and return it's value

                    return manip(*this);
                }	

                //  define the custom endl.
                static Debug::CoutBase& endl(Debug::CoutBase& stream) {
                    if (stream.enabled) {
                        stream.buffer << std::endl;
                    }
                    return stream;
                }

                // this is the type of std::cout
                typedef std::basic_ostream<char, std::char_traits<char> > CoutType;

                // this is the function signature of std::endl
                typedef CoutType& (*StandardEndLine)(CoutType&);

                Debug::CoutBase& operator<<(StandardEndLine manip) {
                    // call the function, but we cannot return it's value

                    if (this->enabled) {
                        manip(buffer);

                        std::cout << buffer.str();
                        buffer.str("");
                    }

                    return *this;
                }	
                Debug::CoutBase& operator<< (const LogType l) {
                    if (this->log_mask & l) {
                        this->setEnabled(true);
                    } else {
                        this->setEnabled(false);
                    }
                    return *this;
                }
        };

        template <class T> class Cout : public CoutBase {
            private:
                T*   ptr;

            public:
                Cout() : CoutBase(0) {
                    this->ptr       = NULL;
                }

                void setSource(T* p) {
                    this->ptr = p;
                }
                T* getSource() {
                    return this->ptr;
                }
        };

        std::map<std::string,CoutBase*> reg;

		Debug() {
			this->enabled   = true;
		}

        ~Debug() {
            for(auto it=this->reg.begin();it!=this->reg.end();it++) {
                CoutBase* c = (*it).second;
                delete(c);
            }
        }

        template <class T> void registerClass(unsigned int log_mask = 0) {
            std::string type = demangle(typeid(T).name());

            if (this->reg.find(type) == this->reg.end()) {
                CoutBase* c = new Cout<T>();
                c->setLogMask(log_mask);
                this->reg[type] = c;
            } else {
                this->reg[type]->setLogMask(log_mask);
            }
        }

        static Debug& getInstance() {
            if (Debug::d == NULL) {
                Debug::d = new Debug();
            }
            return *Debug::d;
        }

        template <class T> static Debug::CoutBase& getInstance(T* p) {
            std::string ti = Debug::d->demangle(typeid(T).name());

            if (Debug::d == NULL) {
                 Debug::d = new Debug();
            }

            CoutBase* cout = NULL;
            if (Debug::d->reg.find(ti) == Debug::d->reg.end()) {
                Debug::d->registerClass<T>(0);
                cout = Debug::d->reg[ti];
                ((Cout<T>*)cout)->setSource(p);
                cout->setEnabled(false);
            } else {
                cout = Debug::d->reg[ti];
            }

            return *cout;
        }
};

#define debug Debug::getInstance(this)

#define RegisterDebugClass(T,...)  namespace {\
    struct __X {\
        __X() {\
            Debug::getInstance().registerClass<T>(__VA_ARGS__);\
        }\
    };\
    __X reg;\
};

#define ALL    Debug::ALL
#define NONE   Debug::NONE
#define INFO   Debug::INFO
#define WARN   Debug::WARN
#define CRIT   Debug::CRITICAL
#define DEBUG1 Debug::DEBUG1
#define DEBUG2 Debug::DEBUG2
#define DEBUG3 Debug::DEBUG3



#endif
