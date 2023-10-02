#include "base/abc/abc.h"
#include "base/main/main.h"
#include "base/main/mainInt.h"
#include <iostream>
#include <fstream>
#include <vector>

/* -> Some hints about PA#1

1. You are not allowed to call the function "Cudd_Eval"
2. The variable order is the same as defined in the BLIF file. You can use "Abc_NtkForEachPi" to traverse all PIs.
3. Please be aware that the variable order may be changed after transforming the network into BDD. 
Hint: Since the "show_bdd" command shows the variable name of each decision level, 
you may look into the source code of that command to find some information.

1. To find the BDD node associated with each PO, use the codes below.

Abc_NtkForEachPo(pNtk, pPo, ithPo) {
    Abc_Obj_t* pRoot = Abc_ObjFanin0(pPo); 
    assert( Abc_NtkIsBddLogic(pRoot->pNtk) );
    DdManager * dd = (DdManager *)pRoot->pNtk->pManFunc;  
    DdNode* ddnode = (DdNode *)pRoot->pData;
}


2. To find the variable order of the BDD, you may use the following codes to find the variable name array.

char** vNamesIn = (char**) Abc_NodeGetFaninNames(pRoot)->pArray;

*/

static int Lsv_CommandPrintNodes      ( Abc_Frame_t *pAbc, int argc, char **argv );
static int Abc_CommandHello           ( Abc_Frame_t *pAbc, int argc, char **argv );
static int Abc_CommandLSVSimBDD       ( Abc_Frame_t *pAbc, int argc, char **argv );
static int Abc_CommandLSVPallSimAIG   ( Abc_Frame_t *pAbc, int argc, char **argv );
void conductSimulation(Abc_Ntk_t *pNtk, Abc_Obj_t *pObj, bool simulaitonAct[], unsigned **simulationarr, int simArrWid);

unsigned int **simulationArr;
bool *simulationAct;

void init(Abc_Frame_t* pAbc) {
    Cmd_CommandAdd(pAbc, "LSV", "lsv_print_nodes", Lsv_CommandPrintNodes, 0);
    /* LSV PA#1 */
    Cmd_CommandAdd( pAbc, "Printing",     "hello",         Abc_CommandHello,            0 );
    Cmd_CommandAdd( pAbc, "Various",      "lsv_sim_bdd",   Abc_CommandLSVSimBDD,        0 );
    Cmd_CommandAdd( pAbc, "Various",      "lsv_sim_aig",   Abc_CommandLSVPallSimAIG,    0 );
    /* */
}

void destroy(Abc_Frame_t* pAbc) {}

Abc_FrameInitializer_t frame_initializer = {init, destroy};

struct PackageRegistrationManager {
  PackageRegistrationManager() { Abc_FrameAddInitializer(&frame_initializer); }
} lsvPackageRegistrationManager;

void Lsv_NtkPrintNodes(Abc_Ntk_t* pNtk) {
  Abc_Obj_t* pObj;
  int i;
  Abc_NtkForEachNode(pNtk, pObj, i) {
    printf("Object Id = %d, name = %s\n", Abc_ObjId(pObj), Abc_ObjName(pObj));
    Abc_Obj_t* pFanin;
    int j;
    Abc_ObjForEachFanin(pObj, pFanin, j) {
      printf("  Fanin-%d: Id = %d, name = %s\n", j, Abc_ObjId(pFanin),
             Abc_ObjName(pFanin));
    }
    if (Abc_NtkHasSop(pNtk)) {
      printf("The SOP of this node:\n%s", (char*)pObj->pData);
    }
  }
}

int Lsv_CommandPrintNodes(Abc_Frame_t* pAbc, int argc, char** argv) {
  Abc_Ntk_t* pNtk = Abc_FrameReadNtk(pAbc);
  int c;
  Extra_UtilGetoptReset();
  while ((c = Extra_UtilGetopt(argc, argv, "h")) != EOF) {
    switch (c) {
      case 'h':
        goto usage;
      default:
        goto usage;
    }
  }
  if (!pNtk) {
    Abc_Print(-1, "Empty network.\n");
    return 1;
  }
  Lsv_NtkPrintNodes(pNtk);
  return 0;

usage:
  Abc_Print(-2, "usage: lsv_print_nodes [-h]\n");
  Abc_Print(-2, "\t        prints the nodes in the network\n");
  Abc_Print(-2, "\t-h    : print the command usage\n");
  return 1;
}

static int Abc_CommandHello ( Abc_Frame_t *pAbc, int argc, char **argv ){
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;

	Abc_Obj_t *thing;
	int i;

    pNtk = Abc_FrameReadNtk(pAbc);

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }
    if ( !Abc_NtkIsStrash(pNtk))
    {
        Abc_Print( -1, "This command must be used under AIG.\n" );
        return 1;
    }
	
	Abc_NtkForEachObj(pNtk, thing, i){
		printf("Object Id: %5d\t", thing->Id);
		printf("Object layer = %3d\n", thing->Level);
	}
    return 0;

usage:
    Abc_Print( -2, "usage: print_stats [-fbdltmpgscuh]\n" );
    Abc_Print( -2, "\t        prints the network internal AIG structure\n" );
    Abc_Print( -2, "\t-h    : print the command usage\n");
    return 1;
}

static int Abc_CommandLSVSimBDD       ( Abc_Frame_t *pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;

    pNtk = Abc_FrameReadNtk(pAbc);

	// printf("Printing args(%d): ", argc);
	// for(int i = 0; i < argc; ++i){
	// 	printf("%s ", argv[i]);
	// }
	// printf("\n");
	// printf("This networks has Pi/Po = %d/%d\n", Abc_NtkPiNum(pNtk), Abc_NtkPoNum(pNtk));

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
        case 'h':
            goto usage;
        default:
            goto usage;
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }

	Abc_Obj_t *pPi;
	int ithPi;

	Abc_Obj_t *pPo;
	int ithPo;
	Abc_NtkForEachPo(pNtk, pPo, ithPo){
		printf("%s: ",Abc_ObjName(pPo) );
		// printf("Po node fanin/fanout : %d / %d\n",Abc_ObjFaninNum(pPo) , Abc_ObjFanoutNum(pPo));

		Abc_Obj_t* pRoot = Abc_ObjFanin0(pPo);
		assert( Abc_NtkIsBddLogic(pRoot->pNtk) );


		// Abc_Obj_t *pFanin;
		// int ithFanin;
		// Abc_ObjForEachFanin(pRoot, pFanin, ithFanin){
		// 	printf("Fanin: %d\n",pFanin->Id);
		// 	char** vNamesIn = (char**) Abc_NodeGetFaninNames(pRoot)->pArray;
		// 	for(int i  = 0; i < Abc_NodeGetFaninNames(pRoot)->nSize; ++i){
		// 		printf("%s ", vNamesIn[i]);
		// 	}
		// 	printf("\n");
		// }

		// char** vNamesIn = (char**) Abc_NodeGetFaninNames(pRoot)->pArray;
		// for(int i  = 0; i < Abc_NodeGetFaninNames(pRoot)->nSize; ++i){
		// 	printf("%s ", vNamesIn[i]);
		// }
		// printf("\n");

		DdManager *manager = (DdManager *)pRoot->pNtk->pManFunc;  
		DdNode* ddnode = (DdNode *)pRoot->pData;

		DdNode* workingDD = ddnode;

		Vec_Int_t *pRootIdxArr = Abc_ObjFaninVec(pRoot);
		// printf("[");
		for(int i = 0; i < pRootIdxArr->nSize; ++i){
			// printf("*%d*", pRootIdxArr->pArray[i]);
			char inputChar = argv[1][pRootIdxArr->pArray[i] - 1];
			if(inputChar == '1'){
				// printf("Do: %d= 1, ",pRootIdxArr->pArray[i] - 1);
				workingDD = Cudd_Cofactor(manager, workingDD, Cudd_bddIthVar(manager, i));

			}else if(inputChar == '0'){
				// printf("Do: %d= 0, ",pRootIdxArr->pArray[i] - 1);
				workingDD = Cudd_Cofactor(manager, workingDD, Cudd_Not(Cudd_bddIthVar(manager, i)));
			}else{
				printf("Strange Things happened!\n");
			}
			
		}

		// printf("]");
		
		if(!Cudd_IsNonConstant(workingDD)){
			if(workingDD == Cudd_ReadLogicZero(manager)) printf("0\n");
			else if (workingDD == Cudd_ReadOne(manager)) printf("1\n");
			else printf("Fail! not 1 and not 0\n");
		}else{
			printf("Fail, not constant!\n");
		}

	}

    return 0;

usage:
    Abc_Print( -2, "usage: lsv_sim_bdd [-h] <input_pattern> \n" );
    Abc_Print( -2, "\t        simulations for a given BDD and an input pattern\n" );
    Abc_Print( -2, "\t-h    : print the command usage\n");
    return 1;
}

static int Abc_CommandLSVPallSimAIG       ( Abc_Frame_t *pAbc, int argc, char **argv )
{
    Abc_Ntk_t * pNtk = Abc_FrameReadNtk(pAbc);
    int c;

    pNtk = Abc_FrameReadNtk(pAbc);

    Extra_UtilGetoptReset();
    while ( ( c = Extra_UtilGetopt( argc, argv, "h" ) ) != EOF )
    {
        switch ( c )
        {
			case 'h':{
				Abc_Print( -2, "usage: lsv_sim_bdd [-h] <input_pattern_file> \n" );
				Abc_Print( -2, "\t        32-bit parallel simulations for a given AIG and some input patterns\n" );
				Abc_Print( -2, "\t-h    : print the command usage\n");
				return 1;
				break;
			}
			default:{
				Abc_Print( -2, "usage: lsv_sim_bdd [-h] <input_pattern_file> \n" );
				Abc_Print( -2, "\t        32-bit parallel simulations for a given AIG and some input patterns\n" );
				Abc_Print( -2, "\t-h    : print the command usage\n");
				return 1;
			}
        }
    }

    if ( pNtk == NULL )
    {
        Abc_Print( -1, "Empty network.\n" );
        return 1;
    }

	// Start writing from below > <

	assert(argc == 2);


	std::ifstream ifs;
	ifs.open(argv[1]);
	std::string str;

	bool initialized = false;
	int inputVars = -1;
	int stimulusSize = 0;
	std::vector<std::vector<unsigned int>> inputs;
	
	while(!ifs.eof()){
		ifs >> str;
		if(!initialized){
			initialized = true;
			inputVars = str.length();
			for(int i = 0; i < inputVars; ++i){
				inputs.push_back(std::vector<unsigned>());
			}
		}
		if((stimulusSize%32) == 0){
			for (int i = 0; i < inputVars; i++){
				inputs[i].push_back(((unsigned)0));
			}

		}
		unsigned filter = ((unsigned) 1) << (stimulusSize % 32);
		// std::cout << stimulusSize << "] " << str;
		// printf("Filter = %x\n", filter);
		for(int i = 0; i < inputVars; ++i){
			if(str[i] == '1'){
				inputs[i][stimulusSize/32] |= filter;
			}
		}
		// for(int i = 0; i < inputVars; ++i){
		// 	for(int j = 0; j < inputs[i].size(); ++j){
		// 		printf("%x, ", inputs[i][j]);
		// 	}		
		// 	printf("\n");
		// }

		stimulusSize++;
	}

	// printf("inputVars = %d, Ssize = %d\n", inputVars, stimulusSize);
	// for(int i = 0; i < inputVars; ++i){
	// 	for(int j = 0; j < inputs[i].size(); ++j){
	// 		printf("%x, ", inputs[i][j]);
	// 	}		
	// 	printf("\n");
	// }

	int simArrLen = Vec_PtrSize((pNtk)->vObjs);
	int simArrWid = (stimulusSize % 32)? (stimulusSize/32 + 1): stimulusSize/32;
	
	// Arrays to record the simulation values and initialization 

	simulationArr = new unsigned int *[simArrLen];
	for(int i = 0; i < simArrLen; ++i){
		simulationArr[i] = new unsigned int [simArrWid];
	}

	// bool simulationAct[simArrLen];
	simulationAct = new bool [simArrLen];
	for(int i = 0; i < simArrLen; ++i){
		simulationAct[i] = false;
	}
	
	// Initialize the Obj #1: constant nodes
	Abc_Obj_t *pConst = Abc_NtkObj(pNtk, 0);
	if(Abc_ObjType(pConst) == ABC_OBJ_CONST1){
		simulationAct[0] = true;
		for(int i  = 0; i < simArrWid; ++i){
			simulationArr[0][i] = (unsigned)0xffffffff;
		}
	}

	// Initialize the Pis
	Abc_Obj_t *pInitPis;
	int ithInitPis;
	Abc_NtkForEachPi(pNtk, pInitPis, ithInitPis){
		// std::cout << "ithInitPis + 1 = " <<  ithInitPis + 1 << "equal TRUE\n";
		simulationAct[ithInitPis + 1] = true;
		for(int i = 0; i < simArrWid; ++i){
			simulationArr[ithInitPis + 1][i] = inputs[ithInitPis][i];
		}
	}

	// // Printing all initialized stuff;
	// printf("Printing all initialized stuff:\n");
	// for(int i = 0; i < simArrLen; ++i){
	// 	if(simulationAct[i]){
	// 		printf("Id = %02d]:", i);
	// 		for(int j = 0; j < simArrWid; ++j){
	// 			printf("%x ", simulationArr[i][j]);
	// 		}
	// 		std::cout << std::endl;
	// 	}
	// }
	Abc_Obj_t *pInternal;
	int ithInternal;
	Abc_NtkForEachNode(pNtk, pInternal, ithInternal){
		int nodeId = (int)Abc_ObjId(pInternal);
		if(simulationAct[nodeId] == false){
			conductSimulation(pNtk, pInternal, simulationAct, simulationArr, simArrWid);
		}
	}

	// Printing all initialized stuff;
	// printf("This is final printing:\n");
	// for(int i = 0; i < simArrLen; ++i){
	// 	if(simulationAct[i]){
	// 		printf("Id = %02d]:", i);
	// 		for(int j = 0; j < simArrWid; ++j){
	// 			printf("%x ", simulationArr[i][j]);
	// 		}
	// 		printf("\n");
	// 	}
	// }

	//Output Answer:
	Abc_Obj_t *pPo;
	int ithPo;
	Abc_NtkForEachPo(pNtk, pPo, ithPo){
		unsigned representNodeId = Abc_ObjFanin0(pPo)->Id;
		
		printf("%s: ",Abc_ObjName(pPo) );
		int bucket, noElement;
		unsigned filter;
		for(int i = 0; i < stimulusSize; ++i){
			bucket = i / 32;
			noElement = i % 32;
			filter = ((unsigned)1) << (noElement);

			unsigned thingy = simulationArr[representNodeId][bucket] & filter;

			if(thingy != 0){
				if(Abc_ObjFaninC0(pPo) == 0) printf("1");
				else printf("0");
			}else{
				if(Abc_ObjFaninC0(pPo) == 0) printf("0");
				else printf("1");

			}
			
		}
		// for(int j = 0; j < simArrWid; ++j){
		// 	printf("%x ", simulationArr[representNodeId][j]);
		// }
		printf("\n");
	}

	// Abc_Obj_t *pObj;
	// int ithObj;
	// Abc_NtkForEachNode(pNtk, pObj, ithObj){
	// 	std::cout << "[" << ithObj << "]";
	// 	printf("%u %u %d", Abc_ObjType(pObj), Abc_ObjId(pObj), Abc_ObjLevel(pObj));
	// 	printf("Left: |%d|%u(%u), |%d|Right: %u(%u)\n", Abc_ObjFaninC0(pObj), Abc_ObjFanin0(pObj)->Id, Abc_ObjFanin0(pObj)->Type, 
	// 	Abc_ObjFaninC1(pObj), Abc_ObjFanin1(pObj)->Id, Abc_ObjFanin1(pObj)->Type);
	// }

	// Abc_NtkForEachObj(pNtk, pObj, ithObj){
	// 	std::cout << "[" << ithObj << "]";
	// 	printf("%u %u %d\n", Abc_ObjType(pObj), Abc_ObjId(pObj), Abc_ObjLevel(pObj));
	// }
	// Abc_NtkForEachPi(pNtk, pObj, ithObj){
	// 	std::cout << "PI" << ithObj << ")";
	// 	printf("%u %u %d\n", Abc_ObjType(pObj), Abc_ObjId(pObj), Abc_ObjLevel(pObj));
	// }
	// Abc_NtkForEachPo(pNtk, pObj, ithObj){
	// 	std::cout << "Po" << ithObj << ")";
	// 	printf("%u %u %d\n", Abc_ObjType(pObj), Abc_ObjId(pObj), Abc_ObjLevel(pObj));
	// }
	ifs.close();
	delete [] simulationAct;
	for(int i = 0; i < simArrLen; ++i){
		delete [] simulationArr[i];
	}
	delete [] simulationArr;
    return 0;

}

void conductSimulation(Abc_Ntk_t *pNtk, Abc_Obj_t *pObj, bool simulaitonAct[], unsigned **simulationarr, int simArrWid){
	
	unsigned thisNodeId = Abc_ObjId(pObj);
	Abc_Obj_t *leftFanin = Abc_ObjFanin0(pObj);
	Abc_Obj_t *rightFanin = Abc_ObjFanin1(pObj);
	unsigned leftIdx = leftFanin->Id;
	unsigned rightIdx = rightFanin->Id;

	if(!simulaitonAct[leftIdx]) conductSimulation(pNtk, leftFanin, simulaitonAct, simulationarr, simArrWid);
	if(!simulaitonAct[rightIdx]) conductSimulation(pNtk, rightFanin, simulaitonAct, simulationarr, simArrWid);

	// printf("Doing %u -> %u <= %u", leftIdx, thisNodeId, rightIdx);
	
	simulaitonAct[(int)thisNodeId] = true;
	
	if((Abc_ObjFaninC0(pObj) == 0) && (Abc_ObjFaninC1(pObj) == 0)){
		for(int i = 0; i < simArrWid; ++i){
			simulationarr[thisNodeId][i] = simulationarr[leftIdx][i] & simulationarr[rightIdx][i];
		}
	}else if((Abc_ObjFaninC0(pObj) == 0) && (Abc_ObjFaninC1(pObj) == 1)){
		for(int i = 0; i < simArrWid; ++i){
			simulationarr[thisNodeId][i] = simulationarr[leftIdx][i] & (~simulationarr[rightIdx][i]);
		}
	}else if((Abc_ObjFaninC0(pObj) == 1) && (Abc_ObjFaninC1(pObj) == 0)){
		for(int i = 0; i < simArrWid; ++i){
			simulationarr[thisNodeId][i] = (~simulationarr[leftIdx][i]) & simulationarr[rightIdx][i];
		}
	}else{ // 11
		for(int i = 0; i < simArrWid; ++i){
			simulationarr[thisNodeId][i] = (~simulationarr[leftIdx][i]) & (~simulationarr[rightIdx][i]);
		}

	}
	
}
