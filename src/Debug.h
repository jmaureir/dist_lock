#ifndef __C_DEBUG__
#define __C_DEBUG__

#include <iostream>

#include <mutex>         
#include <atomic>

class Debug {

	private:
		std::atomic<bool> locked;
		std::atomic<bool> enabled;
		uint64_t id;

	public:

		static std::mutex mtx;

		Debug() {
			this->id      = 0;
			this->enabled = true;
		}

		void setDebugId(uint64_t id) {
			this->id = id;
		}

		void debugEnabled(bool flag) {
			this->enabled = flag;
		}

		bool isDebugEnable() {
			return this->enabled;
		}

		template <typename T> Debug& operator<< (const T& o) {
			if (!this->locked) {
                //std::cerr << "debug adquiring lock" << std::endl;

				Debug::mtx.lock();
				this->locked = true;
			}

			if (this->enabled) {
				std::cout << o;
			}
			return *this;
		}

		// function that takes a custom stream, and returns it
		typedef Debug& (*MyStreamManipulator)(Debug&);

		// function with a custom signature	
		Debug& operator<<(MyStreamManipulator manip) {
			// call the function, and return it's value

			return manip(*this);
		    }	

		//  define the custom endl.
		static Debug& endl(Debug& stream) {
			if (stream.enabled) {
				std::cout << std::endl;
			}
        	return stream;
		}

		// this is the type of std::cout
		typedef std::basic_ostream<char, std::char_traits<char> > CoutType;

		// this is the function signature of std::endl
		typedef CoutType& (*StandardEndLine)(CoutType&);

		Debug& operator<<(StandardEndLine manip) {
			// call the function, but we cannot return it's value

			if (this->enabled) {
				manip(std::cout);
			}

            //std::cerr << "debug releasing lock" << std::endl;

			this->locked = false;
			Debug::mtx.unlock();

			return *this;
		}	

};

#define debug *this

#endif
