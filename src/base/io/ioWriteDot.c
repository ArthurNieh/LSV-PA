/**CFile****************************************************************

  FileName    [ioWriteDot.c]

  SystemName  [ABC: Logic synthesis and verification system.]

  PackageName [Command processing package.]

  Synopsis    [Procedures to write the graph structure of AIG in DOT.]

  Author      [Alan Mishchenko]
  
  Affiliation [UC Berkeley]

  Date        [Ver. 1.0. Started - June 20, 2005.]

  Revision    [$Id: ioWriteDot.c,v 1.00 2005/06/20 00:00:00 alanmi Exp $]

***********************************************************************/

#include "io.h"
#include "seqInt.h"

////////////////////////////////////////////////////////////////////////
///                        DECLARATIONS                              ///
////////////////////////////////////////////////////////////////////////

static char * Abc_NtkPrintSop( char * pSop );
static int    Abc_NtkCountLogicNodes( Vec_Ptr_t * vNodes );

////////////////////////////////////////////////////////////////////////
///                     FUNCTION DEFINITIONS                         ///
////////////////////////////////////////////////////////////////////////

/**Function*************************************************************

  Synopsis    [Writes the graph structure of AIG for DOT.]

  Description [Useful for graph visualization using tools such as GraphViz: 
  http://www.graphviz.org/]
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteDotAig( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesShow, char * pFileName, int fMulti )
{
    FILE * pFile;
    Abc_Obj_t * pNode, * pTemp, * pPrev;
    int LevelMin, LevelMax, fHasCos, Level, i;
    int Limit = 300;

    assert( Abc_NtkHasAig(pNtk) );

    if ( vNodes->nSize < 1 )
    {
        printf( "The set has no nodes. DOT file is not written.\n" );
        return;
    }

    if ( vNodes->nSize > Limit )
    {
        printf( "The set has more than %d nodes. DOT file is not written.\n", Limit );
        return;
    }

    // start the stream
    if ( (pFile = fopen( pFileName, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", pFileName );
        return;
    }

    // mark the nodes from the set
    Vec_PtrForEachEntry( vNodes, pNode, i )
        pNode->fMarkC = 1;
    if ( vNodesShow )
        Vec_PtrForEachEntry( vNodesShow, pNode, i )
            pNode->fMarkB = 1;

    // find the largest and the smallest levels
    LevelMin = 10000;
    LevelMax = -1;
    fHasCos  = 0;
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( Abc_ObjIsCo(pNode) )
        {
            fHasCos = 1;
            continue;
        }
        if ( LevelMin > (int)pNode->Level )
            LevelMin = pNode->Level;
        if ( LevelMax < (int)pNode->Level )
            LevelMax = pNode->Level;
    }

    // set the level of the CO nodes
    if ( fHasCos )
    {
        LevelMax++;
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( Abc_ObjIsCo(pNode) )
                pNode->Level = LevelMax;
        }
    }

    // write the DOT header
    fprintf( pFile, "# %s\n",  "AIG structure generated by ABC" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "digraph AIG {\n" );
    fprintf( pFile, "size = \"7.5,10\";\n" );
//  fprintf( pFile, "ranksep = 0.5;\n" );
//  fprintf( pFile, "nodesep = 0.5;\n" );
    fprintf( pFile, "center = true;\n" );
//  fprintf( pFile, "orientation = landscape;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//  fprintf( pFile, "edge [dir = none];\n" );
    fprintf( pFile, "edge [dir = back];\n" );
    fprintf( pFile, "\n" );

    // labels on the left of the picture
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  node [shape = plaintext];\n" );
    fprintf( pFile, "  edge [style = invis];\n" );
    fprintf( pFile, "  LevelTitle1 [label=\"\"];\n" );
    fprintf( pFile, "  LevelTitle2 [label=\"\"];\n" );
    // generate node names with labels
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d", Level );
        fprintf( pFile, " [label = " );
        // label name
        fprintf( pFile, "\"" );
        fprintf( pFile, "\"" );
        fprintf( pFile, "];\n" );
    }

    // genetate the sequence of visible/invisible nodes to mark levels
    fprintf( pFile, "  LevelTitle1 ->  LevelTitle2 ->" );
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d",  Level );
        // the connector
        if ( Level != LevelMin )
            fprintf( pFile, " ->" );
        else
            fprintf( pFile, ";" );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate title box on top
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle1;\n" );
    fprintf( pFile, "  title1 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=20,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "%s", "AIG structure visualized by ABC" );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "Benchmark \\\"%s\\\". ", pNtk->pName );
    fprintf( pFile, "Time was %s. ",  Extra_TimeStamp() );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate statistics box
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle2;\n" );
    fprintf( pFile, "  title2 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=18,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "The set contains %d logic nodes and spans %d levels.", Abc_NtkCountLogicNodes(vNodes), LevelMax - LevelMin + 1 );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate the POs
    if ( fHasCos )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMax );
        // generate the PO nodes
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCo(pNode) )
                continue;
            fprintf( pFile, "  Node%d%s [label = \"%s%s\"", pNode->Id, 
                (Abc_ObjIsLatch(pNode)? "_in":""), Abc_ObjName(pNode), (Abc_ObjIsLatch(pNode)? "_in":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsLatch(pNode)? "box":"invtriangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate nodes of each rank
    for ( Level = LevelMax - fHasCos; Level >= LevelMin && Level > 0; Level-- )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  Level );
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( (int)pNode->Level != Level )
                continue;
            fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Id, pNode->Id );
            fprintf( pFile, ", shape = ellipse" );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate the PI nodes if any
    if ( LevelMin == 0 )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMin );
        // generat the PO nodes
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCi(pNode) )
            {
                // check if the costant node is present
                if ( Abc_ObjFaninNum(pNode) == 0 && Abc_ObjFanoutNum(pNode) > 0 )
                {
                    fprintf( pFile, "  Node%d [label = \"Const1\"", pNode->Id );
                    fprintf( pFile, ", shape = ellipse" );
                    if ( pNode->fMarkB )
                        fprintf( pFile, ", style = filled" );
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
                    fprintf( pFile, "];\n" );
                }
                continue;
            }
            fprintf( pFile, "  Node%d%s [label = \"%s%s\"", pNode->Id, 
                (Abc_ObjIsLatch(pNode)? "_out":""), Abc_ObjName(pNode), (Abc_ObjIsLatch(pNode)? "_out":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsLatch(pNode)? "box":"triangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate invisible edges from the square down
    fprintf( pFile, "title1 -> title2 [style = invis];\n" );
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        fprintf( pFile, "title2 -> Node%d%s [style = invis];\n", pNode->Id, 
            (Abc_ObjIsLatch(pNode)? "_in":"") );
    }

    // generate edges
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( Abc_ObjFaninNum(pNode) == 0 )
            continue;
        if ( fMulti && Abc_ObjIsNode(pNode) )
        {
            Vec_Ptr_t * vSuper;
            Abc_Obj_t * pFanin;
            int k, fCompl;
            vSuper = (Vec_Ptr_t *)pNode->pCopy;
            assert( vSuper != NULL );
            Vec_PtrForEachEntry( vSuper, pFanin, k )
            {
                fCompl = Abc_ObjIsComplement(pFanin);
                pFanin = Abc_ObjRegular(pFanin);
                if ( !pFanin->fMarkC )
                    continue;
                fprintf( pFile, "Node%d",  pNode->Id );
                fprintf( pFile, " -> " );
                fprintf( pFile, "Node%d%s",  pFanin->Id, (Abc_ObjIsLatch(pFanin)? "_out":"") );
                fprintf( pFile, " [" );
                fprintf( pFile, "style = %s", fCompl? "dotted" : "bold" );
                fprintf( pFile, "]" );
                fprintf( pFile, ";\n" );
            }
            continue;
        }
        // generate the edge from this node to the next
        if ( Abc_ObjFanin0(pNode)->fMarkC )
        {
            fprintf( pFile, "Node%d%s",  pNode->Id, (Abc_ObjIsLatch(pNode)? "_in":"") );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d%s",  Abc_ObjFaninId0(pNode), (Abc_ObjIsLatch(Abc_ObjFanin0(pNode))? "_out":"") );
            fprintf( pFile, " [" );
            fprintf( pFile, "style = %s", Abc_ObjFaninC0(pNode)? "dotted" : "bold" );
            if ( Abc_NtkIsSeq(pNode->pNtk) && Seq_ObjFaninL0(pNode) > 0 )
            fprintf( pFile, ", label = \"%s\"", Seq_ObjFaninGetInitPrintable(pNode,0) );
            fprintf( pFile, "]" );
            fprintf( pFile, ";\n" );
        }
        if ( Abc_ObjFaninNum(pNode) == 1 )
            continue;
        // generate the edge from this node to the next
        if ( Abc_ObjFanin1(pNode)->fMarkC )
        {
            fprintf( pFile, "Node%d",  pNode->Id );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d%s",  Abc_ObjFaninId1(pNode), (Abc_ObjIsLatch(Abc_ObjFanin1(pNode))? "_out":"") );
            fprintf( pFile, " [" );
            fprintf( pFile, "style = %s", Abc_ObjFaninC1(pNode)? "dotted" : "bold" );
            if ( Abc_NtkIsSeq(pNode->pNtk) && Seq_ObjFaninL1(pNode) > 0 )
            fprintf( pFile, ", label = \"%s\"", Seq_ObjFaninGetInitPrintable(pNode,1) );
            fprintf( pFile, "]" );
            fprintf( pFile, ";\n" );
        }
        // generate the edges between the equivalent nodes
        pPrev = pNode;
        for ( pTemp = pNode->pData; pTemp; pTemp = pTemp->pData )
        {
            if ( pTemp->fMarkC )
            {
                fprintf( pFile, "Node%d",  pPrev->Id );
                fprintf( pFile, " -> " );
                fprintf( pFile, "Node%d",  pTemp->Id );
                fprintf( pFile, " [style = %s]", (pPrev->fPhase ^ pTemp->fPhase)? "dotted" : "bold" );
                fprintf( pFile, ";\n" );
                pPrev = pTemp;
            }
        }
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    // unmark the nodes from the set
    Vec_PtrForEachEntry( vNodes, pNode, i )
        pNode->fMarkC = 0;
    if ( vNodesShow )
        Vec_PtrForEachEntry( vNodesShow, pNode, i )
            pNode->fMarkB = 0;
}

/**Function*************************************************************

  Synopsis    [Writes the graph structure of network for DOT.]

  Description [Useful for graph visualization using tools such as GraphViz: 
  http://www.graphviz.org/]
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
void Io_WriteDotNtk( Abc_Ntk_t * pNtk, Vec_Ptr_t * vNodes, Vec_Ptr_t * vNodesShow, char * pFileName, int fGateNames )
{
    FILE * pFile;
    Abc_Obj_t * pNode, * pFanin;
    char * pSopString;
    int LevelMin, LevelMax, fHasCos, Level, i, k, fHasBdds;
    int Limit = 300;

    assert( !Abc_NtkHasAig(pNtk) );

    if ( vNodes->nSize < 1 )
    {
        printf( "The set has no nodes. DOT file is not written.\n" );
        return;
    }

    if ( vNodes->nSize > Limit )
    {
        printf( "The set has more than %d nodes. DOT file is not written.\n", Limit );
        return;
    }

    // start the stream
    if ( (pFile = fopen( pFileName, "w" )) == NULL )
    {
        fprintf( stdout, "Cannot open the intermediate file \"%s\".\n", pFileName );
        return;
    }

    // transform logic functions from BDD to SOP
    if ( fHasBdds = Abc_NtkIsBddLogic(pNtk) )
        Abc_NtkBddToSop(pNtk, 0);

    // mark the nodes from the set
    Vec_PtrForEachEntry( vNodes, pNode, i )
        pNode->fMarkC = 1;
    if ( vNodesShow )
        Vec_PtrForEachEntry( vNodesShow, pNode, i )
            pNode->fMarkB = 1;

    // get the levels of nodes
    Abc_NtkGetLevelNum( pNtk );

    // find the largest and the smallest levels
    LevelMin = 10000;
    LevelMax = -1;
    fHasCos  = 0;
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( Abc_ObjIsCo(pNode) )
        {
            fHasCos = 1;
            continue;
        }
        if ( LevelMin > (int)pNode->Level )
            LevelMin = pNode->Level;
        if ( LevelMax < (int)pNode->Level )
            LevelMax = pNode->Level;
    }

    // set the level of the CO nodes
    if ( fHasCos )
    {
        LevelMax++;
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( Abc_ObjIsCo(pNode) )
                pNode->Level = LevelMax;
        }
    }

    // write the DOT header
    fprintf( pFile, "# %s\n",  "Network structure generated by ABC" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "digraph AIG {\n" );
    fprintf( pFile, "size = \"7.5,10\";\n" );
//  fprintf( pFile, "ranksep = 0.5;\n" );
//  fprintf( pFile, "nodesep = 0.5;\n" );
    fprintf( pFile, "center = true;\n" );
//  fprintf( pFile, "orientation = landscape;\n" );
//  fprintf( pFile, "edge [fontsize = 10];\n" );
//  fprintf( pFile, "edge [dir = none];\n" );
    fprintf( pFile, "edge [dir = back];\n" );
    fprintf( pFile, "\n" );

    // labels on the left of the picture
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  node [shape = plaintext];\n" );
    fprintf( pFile, "  edge [style = invis];\n" );
    fprintf( pFile, "  LevelTitle1 [label=\"\"];\n" );
    fprintf( pFile, "  LevelTitle2 [label=\"\"];\n" );
    // generate node names with labels
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d", Level );
        fprintf( pFile, " [label = " );
        // label name
        fprintf( pFile, "\"" );
        fprintf( pFile, "\"" );
        fprintf( pFile, "];\n" );
    }

    // genetate the sequence of visible/invisible nodes to mark levels
    fprintf( pFile, "  LevelTitle1 ->  LevelTitle2 ->" );
    for ( Level = LevelMax; Level >= LevelMin; Level-- )
    {
        // the visible node name
        fprintf( pFile, "  Level%d",  Level );
        // the connector
        if ( Level != LevelMin )
            fprintf( pFile, " ->" );
        else
            fprintf( pFile, ";" );
    }
    fprintf( pFile, "\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate title box on top
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle1;\n" );
    fprintf( pFile, "  title1 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=20,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "%s", "Network structure visualized by ABC" );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "Benchmark \\\"%s\\\". ", pNtk->pName );
    fprintf( pFile, "Time was %s. ",  Extra_TimeStamp() );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate statistics box
    fprintf( pFile, "{\n" );
    fprintf( pFile, "  rank = same;\n" );
    fprintf( pFile, "  LevelTitle2;\n" );
    fprintf( pFile, "  title2 [shape=plaintext,\n" );
    fprintf( pFile, "          fontsize=18,\n" );
    fprintf( pFile, "          fontname = \"Times-Roman\",\n" );
    fprintf( pFile, "          label=\"" );
    fprintf( pFile, "The set contains %d logic nodes and spans %d levels.", Abc_NtkCountLogicNodes(vNodes), LevelMax - LevelMin + 1 );
    fprintf( pFile, "\\n" );
    fprintf( pFile, "\"\n" );
    fprintf( pFile, "         ];\n" );
    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );

    // generate the POs
    if ( fHasCos )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMax );
        // generate the PO nodes
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCo(pNode) )
                continue;
            fprintf( pFile, "  Node%d%s [label = \"%s%s\"", pNode->Id, 
                (Abc_ObjIsLatch(pNode)? "_in":""), Abc_ObjName(pNode), (Abc_ObjIsLatch(pNode)? "_in":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsLatch(pNode)? "box":"invtriangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate nodes of each rank
    for ( Level = LevelMax - fHasCos; Level >= LevelMin && Level > 0; Level-- )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  Level );
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( (int)pNode->Level != Level )
                continue;
//            fprintf( pFile, "  Node%d [label = \"%d\"", pNode->Id, pNode->Id );
            if ( Abc_NtkHasMapping(pNtk) && fGateNames )
                pSopString = Mio_GateReadName(pNode->pData);
            else if ( Abc_NtkHasMapping(pNtk) )
                pSopString = Abc_NtkPrintSop(Mio_GateReadSop(pNode->pData));
            else
                pSopString = Abc_NtkPrintSop(pNode->pData);
            fprintf( pFile, "  Node%d [label = \"%d\\n%s\"", pNode->Id, pNode->Id, pSopString );

            fprintf( pFile, ", shape = ellipse" );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate the PI nodes if any
    if ( LevelMin == 0 )
    {
        fprintf( pFile, "{\n" );
        fprintf( pFile, "  rank = same;\n" );
        // the labeling node of this level
        fprintf( pFile, "  Level%d;\n",  LevelMin );
        // generat the PO nodes
        Vec_PtrForEachEntry( vNodes, pNode, i )
        {
            if ( !Abc_ObjIsCi(pNode) )
            {
                // check if the costant node is present
                if ( Abc_ObjFaninNum(pNode) == 0 && Abc_ObjFanoutNum(pNode) > 0 )
                {
                    fprintf( pFile, "  Node%d [label = \"Const1\"", pNode->Id );
                    fprintf( pFile, ", shape = ellipse" );
                    if ( pNode->fMarkB )
                        fprintf( pFile, ", style = filled" );
                    fprintf( pFile, ", color = coral, fillcolor = coral" );
                    fprintf( pFile, "];\n" );
                }
                continue;
            }
            fprintf( pFile, "  Node%d%s [label = \"%s%s\"", pNode->Id, 
                (Abc_ObjIsLatch(pNode)? "_out":""), Abc_ObjName(pNode), (Abc_ObjIsLatch(pNode)? "_out":"") );
            fprintf( pFile, ", shape = %s", (Abc_ObjIsLatch(pNode)? "box":"triangle") );
            if ( pNode->fMarkB )
                fprintf( pFile, ", style = filled" );
            fprintf( pFile, ", color = coral, fillcolor = coral" );
            fprintf( pFile, "];\n" );
        }
        fprintf( pFile, "}" );
        fprintf( pFile, "\n" );
        fprintf( pFile, "\n" );
    }

    // generate invisible edges from the square down
    fprintf( pFile, "title1 -> title2 [style = invis];\n" );
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        if ( (int)pNode->Level != LevelMax )
            continue;
        fprintf( pFile, "title2 -> Node%d%s [style = invis];\n", pNode->Id, 
            (Abc_ObjIsLatch(pNode)? "_in":"") );
    }

    // generate edges
    Vec_PtrForEachEntry( vNodes, pNode, i )
    {
        Abc_ObjForEachFanin( pNode, pFanin, k )
        {
            if ( !Abc_ObjFanin0(pNode)->fMarkC )
                continue;
            // generate the edge from this node to the next
            fprintf( pFile, "Node%d%s",  pNode->Id, (Abc_ObjIsLatch(pNode)? "_in":"") );
            fprintf( pFile, " -> " );
            fprintf( pFile, "Node%d%s",  Abc_ObjFaninId(pNode,k), (Abc_ObjIsLatch(Abc_ObjFanin(pNode,k))? "_out":"") );
            fprintf( pFile, " [style = bold" );
            fprintf( pFile, ", label = \"%c\"", 'a' + k );
            fprintf( pFile, "]" );
            fprintf( pFile, ";\n" );
        }
    }

    fprintf( pFile, "}" );
    fprintf( pFile, "\n" );
    fprintf( pFile, "\n" );
    fclose( pFile );

    // unmark the nodes from the set
    Vec_PtrForEachEntry( vNodes, pNode, i )
        pNode->fMarkC = 0;
    if ( vNodesShow )
        Vec_PtrForEachEntry( vNodesShow, pNode, i )
            pNode->fMarkB = 0;

    // convert the network back into BDDs if this is how it was
    if ( fHasBdds )
        Abc_NtkSopToBdd(pNtk);
}

/**Function*************************************************************

  Synopsis    [Computes the printable SOP form.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
char * Abc_NtkPrintSop( char * pSop ) 
{
    static char Buffer[1000];
    char * pGet, * pSet;
    pSet = Buffer;
    for ( pGet = pSop; *pGet; pGet++ )
    {        
        if ( *pGet == '\n' )
        {
            *pSet++ = '\\';
            *pSet++ = 'n';
        }
        else
            *pSet++ = *pGet;
    }
    *(pSet-2) = 0;
    return Buffer;
}

/**Function*************************************************************

  Synopsis    [Computes the printable SOP form.]

  Description []
  
  SideEffects []

  SeeAlso     []

***********************************************************************/
int Abc_NtkCountLogicNodes( Vec_Ptr_t * vNodes )
{
    Abc_Obj_t * pObj;
    int i, Counter = 0;
    Vec_PtrForEachEntry( vNodes, pObj, i )
    {
        if ( !Abc_ObjIsNode(pObj) )
            continue;
        if ( Abc_ObjFaninNum(pObj) == 0 && Abc_ObjFanoutNum(pObj) == 0 )
            continue;
        Counter ++;
    }
    return Counter;
}

////////////////////////////////////////////////////////////////////////
///                       END OF FILE                                ///
////////////////////////////////////////////////////////////////////////


