//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "QMCWaveFunctions/JABBuilder.h"
#include "QMCWaveFunctions/PadeJastrow.h"
#include "QMCWaveFunctions/NoCuspJastrow.h"
#include "QMCWaveFunctions/OneBodyJastrowFunction.h"
//#include "QMCWaveFunctions/TwoBodyJastrowFunction.h"

namespace qmcplusplus {

  template<class FN>
  bool JABBuilder::createJAB(xmlNodePtr cur, FN* dummy) {

    int cur_var = targetPsi.VarList.size();
    string corr_tag("correlation");

    vector<FN*> jastrow;
    cur = cur->xmlChildrenNode;
    DistanceTableData* d_table = NULL;
    int ng = 0;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == dtable_tag) {
      	string source_name((const char*)(xmlGetProp(cur,(const xmlChar *)"source")));
        map<string,ParticleSet*>::iterator pa_it(ptclPool.find(source_name));
        if(pa_it == ptclPool.end()) return false;
	ParticleSet* a = (*pa_it).second;
	d_table = DistanceTable::getTable(DistanceTable::add(*a,targetPtcl));
	ng = a->getSpeciesSet().getTotalNum();
	XMLReport("Number of sources " << ng)
	for(int i=0; i<ng; i++) jastrow.push_back(NULL);
      } else if(cname == corr_tag) {
	string speciesA((const char*)(xmlGetProp(cur,(const xmlChar *)"speciesA")));
	//string speciesB((const char*)(xmlGetProp(cur,(const xmlChar *)"speciesB")));
	int ia = d_table->origin().getSpeciesSet().findSpecies(speciesA);
	if(!(jastrow[ia])) {
	  jastrow[ia]= new FN;
	  jastrow[ia]->put(cur,targetPsi.VarList);
	  XMLReport("Added Jastrow Correlation between " << speciesA)
	}
      }
      cur = cur->next;
    } // while cur
    
    typedef OneBodyJastrow<FN> JneType;
    JneType* J1 = new JneType(targetPtcl, d_table);
    for(int ig=0; ig<ng; ig++) {
      J1->addFunc(ig,jastrow[ig]);
    }

    //set this jastrow function to be not optimizable
    if(targetPsi.VarList.size() == cur_var) {
      J1->setOptimizable(false);
    }

    targetPsi.add(J1);
    XMLReport("Added a One-Body Jastrow Function")
    return true;
  }

  bool JABBuilder::put(xmlNodePtr cur) {

    string jastfunction("pade");
    const xmlChar *ftype = xmlGetProp(cur, (const xmlChar *)"function");
    if(ftype) jastfunction = (const char*) ftype;

    bool success=false;
    app_log() << "  One-Body Jastrow Function = " << jastfunction << endl;
    if(jastfunction == "nocusp") {
      NoCuspJastrow<ValueType> *dummy = 0;
      success = createJAB(cur,dummy);
    } else if(jastfunction == "pade") {
      PadeJastrow<ValueType> *dummy = 0;
      success = createJAB(cur,dummy);
    } else if(jastfunction == "pade2") {
      PadeJastrow2<ValueType> *dummy = 0;
      success = createJAB(cur,dummy);
    }
    return success;
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
