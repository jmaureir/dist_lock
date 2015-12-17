
#ifndef __ActionEvent__
#define __ActionEvent__

#include <iostream>
#include <string>
#include <sstream>

class Observable;

class ActionEvent {

	private:
		Observable*     source;
		std::string     theActionCommand;
		int             theEventId;

	public:

		ActionEvent(int id, std::string actionCommand) {
			this->theActionCommand = actionCommand;
			this->theEventId       = id;
		}

		ActionEvent(Observable* s=NULL) {
			this->source             = s;
			this->theActionCommand   = "";
			this->theEventId         = -1;
		}

		virtual ~ActionEvent() {

		}

		int getId() {
			return(this->theEventId);
		}

		std::string getActionCommand() {
			return(this->theActionCommand);
		}

		virtual Observable* getSource() {
			return(this->source);
		}

		virtual void setSource(Observable* obs) {
			this->source = obs;
		}

		virtual std::string toString() {
			std::stringstream tmp;
			tmp << "ActionEvent@" << this;
			return(tmp.str());
		}

		virtual ActionEvent* clone() {
			ActionEvent* etv = new ActionEvent(*this);
			return etv;
		}

		friend std::ostream& operator << (std::ostream& os, ActionEvent& a) {
			os << a.toString();
			return os;
		}
};

#endif
