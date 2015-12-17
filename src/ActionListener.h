/**********************************************************
* File Name : ActionListener.h
*
* Last Modified : Mon 04 Mar 2013 05:28:17 PM CLST
* (c) Juan-Carlos Maureira
* Center for Mathematical Modeling
* University of Chile
***********************************************************/

#ifndef __ActionListener__
#define __ActionListener__

#include "ActionEvent.h"
#include <vector>

typedef std::vector<Observable*> ObservableList;

	
class ActionListener {

	private:
		ObservableList obs_list;

	public:

		ActionListener() {

		}

		~ActionListener() {

		}

		void addObservable(Observable* obs) {
			this->obs_list.push_back(obs);
		}

		void removeObservable(Observable* obs) {

			for(ObservableList::iterator it=this->obs_list.begin();it!=this->obs_list.end();it++) {
				Observable* obs_se = *it;
				if (obs_se == obs) {
					this->obs_list.erase(it);
					break;
				}
			}
		}

		ObservableList& getObservableObjects() {
			return(this->obs_list);
		}

		virtual void actionPerformed(ActionEvent* evt)=0;

};

#endif

