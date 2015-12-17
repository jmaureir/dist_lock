/**********************************************************
* File Name : Observable.h
*
* Last Modified : Mon 04 Mar 2013 05:30:42 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __Observable__
#define __Observable__

#include "ActionListener.h"
#include <vector>

class Observable {

	private:
		typedef std::vector<ActionListener*> ActionListenerList;

		ActionListenerList al_list;

		virtual void _actionPerformed(ActionEvent* evt) {
			evt->setSource(this);
			for(unsigned int i=0;i<this->al_list.size();i++) {
				ActionListener* al = this->al_list[i];

				if (al!=NULL) {
					al->actionPerformed(evt);
				}
			}
		}


	public:
		virtual void addActionListener(ActionListener* al) {
			this->al_list.push_back(al);
			al->addObservable(this);
		}

		virtual void removeActionListener(ActionListener* al) {

			al->removeObservable(this);

			for(ActionListenerList::iterator it=this->al_list.begin();it!=al_list.end();it++) {
				ActionListener* al_tmp = *it;
				if (al_tmp == al) {
					this->al_list.erase(it);
					break;
				}
			}
		}

		void removeAllActionListeners() {
			this->al_list.clear();
		}

		virtual void actionPerformed(ActionEvent* evt) {
			this->_actionPerformed(evt);
			delete(evt);
		}

		virtual void actionWithReturnPerformed(ActionEvent* evt) {
			this->_actionPerformed(evt);
			// do not remove the triggered event
		}

		virtual std::string toString() { 
			std::stringstream tmp;
			tmp << "Observable@" << (void*)this;
			return(tmp.str());
		}

		friend std::ostream& operator << (std::ostream& os, Observable& obs) {
			os << obs.toString();
			return(os);
		}
};

#endif
