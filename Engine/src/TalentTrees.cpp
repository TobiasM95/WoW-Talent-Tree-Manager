#include "TalentTrees.h"

#include <iostream>
#include <vector>
#include <algorithm>
#include <sstream> 
#include <fstream>
#include "Windows.h"
#include <chrono>
#include <thread>

namespace Engine {
    //Tree/talent helper functions

    /*
    DFS cyclicity check of a tree
    */
    bool checkIfTreeHasCycle(TalentTree& tree) {
        TreeCycleCheckFormat tccf = createTreeCycleCheckFormat(tree);
        return checkCyclicity(tccf);
    }

    /*
    DFS cyclicity check of a tree if a talent would be inserted
    */
    bool checkIfTalentInsertsCycle(TalentTree& tree, std::shared_ptr<Talent> talent) {
        TreeCycleCheckFormat tccf = createTreeCycleCheckFormat(tree, talent);
        return checkCyclicity(tccf);
    }

    /*
    DFS cyclicity check of a potential tree generated by a tree representation string
    */
    bool checkIfParseStringProducesCycle(std::string treeRep) {
        TreeCycleCheckFormat tccf = createTreeCycleCheckFormat(treeRep);
        return checkCyclicity(tccf);
    }

    /*
    Helper function to create a simple DFS cyclicity check data structure with a tree
    */
    TreeCycleCheckFormat createTreeCycleCheckFormat(TalentTree& tree) {
        TreeCycleCheckFormat tccf;
        std::map<int, int> indexMap;
        int index = 0;
        for (auto& talent : tree.orderedTalents) {
            indexMap[talent.first] = index;
            index++;
        }
        for (auto& talent : tree.orderedTalents) {
            tccf.talents.push_back(0);
            std::vector<int> childVector;
            for (auto& child : talent.second->children) {
                childVector.push_back(indexMap[child->index]);
            }
            tccf.children.push_back(childVector);
        }
        return tccf;
    }

    /*
    Helper function to create a simple DFS cyclicity check data structure with a tree and a talent
    */
    TreeCycleCheckFormat createTreeCycleCheckFormat(TalentTree& tree, std::shared_ptr<Talent> talent) {
        TreeCycleCheckFormat tccf;
        std::map<int, int> indexMap;
        int index = 0;
        for (auto& talent : tree.orderedTalents) {
            indexMap[talent.first] = index;
            index++;
        }
        for (auto& talent : tree.orderedTalents) {
            tccf.talents.push_back(0);
            std::vector<int> childVector;
            for (auto& child : talent.second->children) {
                childVector.push_back(indexMap[child->index]);
            }
            tccf.children.push_back(childVector);
        }
        tccf.talents.push_back(0);
        std::vector<int> childVector;
        for (auto& child : talent->children) {
            childVector.push_back(indexMap[child->index]);
        }
        tccf.children.push_back(childVector);
        for (auto& parent : talent->parents) {
            tccf.children[indexMap[parent->index]].push_back(tccf.talents.size() - 1);
        }
        return tccf;
    }

    TreeCycleCheckFormat createTreeCycleCheckFormat(std::string treeRep) {
        std::vector<std::string> treeComponents = splitString(treeRep, ";");
        TreeCycleCheckFormat tccf;
        std::map<int, int> indexMap;
        for (int i = 1; i < treeComponents.size(); i++) {
            if (treeComponents[i] == "")
                continue;
            int talentIndex = std::stoi(splitString(treeComponents[i], ":")[0]);
            indexMap[talentIndex] = i - 1;
        }
        for (int i = 1; i < treeComponents.size(); i++) {
            if (treeComponents[i] == "")
                continue;
            tccf.talents.push_back(0);
            std::vector<int> childVector;
            std::vector<std::string> childIndexStrings = splitString(splitString(treeComponents[i], ":")[11], ",");
            for (auto& childIndex : childIndexStrings) {
                if (childIndex == "")
                    continue;
                childVector.push_back(indexMap[std::stoi(childIndex)]);
            }
            tccf.children.push_back(childVector);
        }
        return tccf;
    }

    /*
    Core DFS cyclicity check function, see https://en.wikipedia.org/wiki/Topological_sorting for pseudo code
    */
    bool checkCyclicity(TreeCycleCheckFormat tree) {
        bool hasCycle = false;
        bool done = false;
        while (!hasCycle) {
            done = true;
            for (int i = 0; i < tree.talents.size(); i++) {
                if (tree.talents[i] == 0) {
                    hasCycle = cycleCheckVisitTalent(i, tree.talents, tree.children);
                    done = false;
                    break;
                }
            }
            if (hasCycle || done) 
                break;
        }
        return hasCycle;
    }

    /*
    Auxilliary DFS cyclicity check function, see https://en.wikipedia.org/wiki/Topological_sorting for pseudo code
    */
    bool cycleCheckVisitTalent(int talentIndex, std::vector<int>& talents, std::vector<std::vector<int>> children) {
        bool hasCycle = false;
        if (talents[talentIndex] == 1)
            return true;
        if (talents[talentIndex] == 2)
            return false;
        talents[talentIndex] = 1;
        for (auto& child : children[talentIndex]) {
            hasCycle = cycleCheckVisitTalent(child, talents, children);
            if (hasCycle)
                return true;
        }

        talents[talentIndex] = 2;
        return false;
    }


    //TTMTODO: combine update node count with update ordered talent list
    void updateNodeCountAndMaxTalentPointsAndMaxID(TalentTree& tree) {
        std::unordered_map<int, int> nodeTalentPoints;
        std::unordered_map<int, std::unordered_set<int>> talentIndexInRow;
        int maxID = 0;
        int maxCol = 1;
        for (auto& root : tree.talentRoots) {
            countNodesRecursively(nodeTalentPoints, maxID, maxCol, talentIndexInRow, root);
        }
        tree.nodeCount = nodeTalentPoints.size();
        int maxTalentPoints = 0;
        for (auto& node : nodeTalentPoints) {
            maxTalentPoints += node.second;
        }
        tree.maxTalentPoints = maxTalentPoints;
        tree.maxID = maxID;
        tree.maxCol = maxCol;
        tree.talentsPerRow.clear();
        for (auto& rowIndices : talentIndexInRow) {
            tree.talentsPerRow[rowIndices.first] = rowIndices.second.size();
        }
    }

    void countNodesRecursively(std::unordered_map<int, int>& nodeTalentPoints, int& maxID, int& maxCol, std::unordered_map<int, std::unordered_set<int>>& talentsPerRow, std::shared_ptr<Talent> talent) {
        nodeTalentPoints[talent->index] = talent->maxPoints;
        if (talent->index >= maxID)
            maxID = talent->index + 1;

        if (talent->column + 1> maxCol)
            maxCol = talent->column + 1;

        talentsPerRow[talent->row].insert(talent->index);
        /*if (talentsPerRow.count(talent->row)) {
        }
        else {
            talentsPerRow[talent->row] = std::unordered_set<int>();
        }*/

        for (auto& child : talent->children)
            countNodesRecursively(nodeTalentPoints, maxID, maxCol, talentsPerRow, child);
    }

    void updateOrderedTalentList(TalentTree& tree) {
        for (auto& root : tree.talentRoots) {
            orderTalentsRecursively(tree.orderedTalents, root);
        }
    }
    void orderTalentsRecursively(std::map<int, std::shared_ptr<Talent>>& talentMap, std::shared_ptr<Talent> talent) {
        talentMap[talent->index] = talent;
        for (auto& child : talent->children) {
            orderTalentsRecursively(talentMap, child);
        }
    }

    void addChild(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child) {
        parent->children.push_back(child);
    }
    void addParent(std::shared_ptr<Talent> child, std::shared_ptr<Talent> parent) {
        child->parents.push_back(parent);
    }
    void pairTalents(std::shared_ptr<Talent> parent, std::shared_ptr<Talent> child) {
        parent->children.push_back(child);
        child->parents.push_back(parent);
    }

    /*
    Transforms a skilled talent tree into a string. That string does not contain the tree structure, just the selected talents.
    */
    std::string getTalentString(TalentTree tree) {
        std::unordered_map<std::string, int> treeRepresentation;
        for (auto& root : tree.talentRoots) {
            addTalentAndChildrenToMap(root, treeRepresentation);
        }
        return unorderedMapToString(treeRepresentation, true);
    }

    /*
    Prints some informations about a specific tree.
    */
    void printTree(TalentTree tree) {
        std::cout << tree.name << std::endl;
        std::cout << "Unspent talent points:\t" << tree.unspentTalentPoints << std::endl;
        std::cout << "Spent talent points:\t" << tree.spentTalentPoints << std::endl;
        std::cout << getTalentString(tree) << std::endl;
    }

    /*
    Creates a full tree string representation that's compatible to import trees in parse tree
    */
    std::string createTreeStringRepresentation(TalentTree tree) {
        //Tree definition string: TreeString;TalentString;TalentString;...
        //Tree info string: preset:name:description:loadoutdescription:unspentTalentPoints:spentTalentPoints
        //Talent definition string: index:name:description1,description2,...:type:row:col:points:maxPoints:pointRequirement:talentSwitch:parentIndices:childIndices
        std::stringstream treeRep;
        treeRep << "custom:" << tree.name << ":" << tree.treeDescription << ":" << tree.loadoutDescription << ":" << tree.unspentTalentPoints << ":" << tree.spentTalentPoints << ";";
        for (auto& talent : tree.orderedTalents) {
            treeRep << talent.first << ":" << talent.second->name << ":";
            for (int i = 0; i < talent.second->descriptions.size() - 1; i++) {
                treeRep << talent.second->descriptions[i] << ",";
            }
            treeRep << talent.second->descriptions[talent.second->descriptions.size() - 1] << ":";
            treeRep << static_cast<int>(talent.second->type) << ":" << talent.second->row << ":" << talent.second->column << ":";
            treeRep << talent.second->points << ":" << talent.second->maxPoints << ":" << talent.second->pointsRequired << ":" << talent.second->talentSwitch << ":";
            if (talent.second->parents.size() > 0) {
                for (int i = 0; i < talent.second->parents.size() - 1; i++) {
                    treeRep << talent.second->parents[i]->index << ",";
                }
                treeRep << talent.second->parents[talent.second->parents.size() - 1]->index;
            }
            treeRep << ":";
            if (talent.second->children.size() > 0) {
                for (int i = 0; i < talent.second->children.size() - 1; i++) {
                    treeRep << talent.second->children[i]->index << ",";
                }
                treeRep << talent.second->children[talent.second->children.size() - 1]->index;
            }
            treeRep << ";";
        }
        return treeRep.str();
    }

    /*
    Helper function that adds a talent and its children recursively to a map (and adds talent switch information if existing).
    */
    void addTalentAndChildrenToMap(std::shared_ptr<Talent> talent, std::unordered_map<std::string, int>& treeRepresentation) {
        std::string talentIndex = std::to_string(talent->index);
        if (talent->type == TalentType::SWITCH) {
            talentIndex += std::to_string(talent->talentSwitch);
        }
        treeRepresentation[talentIndex] = talent->points;
        for (int i = 0; i < talent->children.size(); i++) {
            addTalentAndChildrenToMap(talent->children[i], treeRepresentation);
        }
    }

    /*
    Helper function that transforms a map that includes talents and their skill points to their respective string representation.
    */
    std::string unorderedMapToString(const std::unordered_map<std::string, int>& treeRepresentation, bool sortOutput) {
        std::vector<std::string> talentsAndPoints;
        talentsAndPoints.reserve(treeRepresentation.size());
        for (auto& talentRepresentation : treeRepresentation) {
            talentsAndPoints.push_back(talentRepresentation.first + std::to_string(talentRepresentation.second));
        }
        if (sortOutput) {
            std::sort(talentsAndPoints.begin(), talentsAndPoints.end());
        }
        std::stringstream treeString;
        for (auto& talentRepresentation : talentsAndPoints) {
            treeString << talentRepresentation << ";";
        }
        return treeString.str();
    }

    /*
    Helper function that creates a talent with the given index name and max points.
    */
    std::shared_ptr<Talent> createTalent(TalentTree& tree, std::string name, int maxPoints) {
        std::shared_ptr<Talent> talent = std::make_shared<Talent>();
        talent->index = tree.maxID;
        tree.nodeCount++;
        tree.maxTalentPoints += maxPoints;
        tree.maxID++;
        talent->maxPoints = maxPoints;
        talent->row = tree.maxID / 3;
        talent->column = tree.maxID % 3;
        tree.orderedTalents[talent->index] = talent;
        return talent;
    }


    //Tree definition string: TreeString;TalentString;TalentString;...
    //Tree info string: preset:name:description:loadoutdescription:unspentTalentPoints:spentTalentPoints
    //Talent definition string: index:name:description1,description2,...:type:row:col:points:maxPoints:pointRequirement:talentSwitch
    /*
    Parse a tree with a given format string. There are two variants, specified by the preset in the tree info string.
    Given a spec name only the preset and then a string of numbers with the spent talent points as well as a .X where x specifies the switch state is needed.
    Given "custom" as preset, the full tree details have to be given. This is how spec trees are built.
    The definition of a custom tree definition can be seen in parseCustomTree function.
    */
    TalentTree parseTree(std::string treeRep) {
        //TTMTODO: Wrap stuff in here in try except cause users are special
        std::vector<std::string> treeDefinitionParts = splitString(treeRep, ";");
        std::vector<std::string> treeInfoParts = splitString(treeDefinitionParts[0], ":");
        if (treeInfoParts[0] == "custom") {
            if (checkIfParseStringProducesCycle(treeRep))
                throw std::logic_error("Tree rep produces cyclic tree!");
            return parseCustomTree(treeRep);
        }
        else {
            return parseTreeFromPreset(treeRep);
        }
        //TTMTODO: Validate tree function!
    }

    TalentTree parseTreeFromPreset(std::string treeRep) {
        //TTMTODO: Implement presets
        TalentTree tree;
        throw std::logic_error("Not yet implemted");
        return tree;
    }

    /*
    Tree definition string: TreeString;TalentString;TalentString;...
    Tree info string: preset:name:description:loadoutdescription:unspentTalentPoints:spentTalentPoints
    Talent definition string: index:name:description1,description2,...:type:row:col:points:maxPoints:pointRequirement:talentSwitch:parentIndices:childIndices
    */
    TalentTree parseCustomTree(std::string treeRep) {
        std::vector<std::string> treeDefinitionParts = splitString(treeRep, ";");
        std::vector<std::shared_ptr<Talent>> roots;
        std::unordered_map<int, std::shared_ptr<Talent>> talentTree;
        TalentTree tree;

        std::vector<std::string> treeInfoParts = splitString(treeDefinitionParts[0], ":");
        tree.name = treeInfoParts[1];
        tree.treeDescription = treeInfoParts[2];
        tree.loadoutDescription = treeInfoParts[3];
        tree.unspentTalentPoints = std::stoi(treeInfoParts[4]);
        tree.spentTalentPoints = std::stoi(treeInfoParts[5]);

        for (int i = 1; i < treeDefinitionParts.size(); i++) {
            if (treeDefinitionParts[i] == "")
                break;
            std::vector<std::string> talentInfo = splitString(treeDefinitionParts[i], ":");
            std::shared_ptr<Talent> t;
            int talentIndex = std::stoi(talentInfo[0]);
            if (talentTree.count(talentIndex)) {
                t = talentTree[talentIndex];
            }
            else {
                t = std::make_shared<Talent>();
                t->index = talentIndex;
                talentTree[t->index] = t;
            }
            std::vector<std::string> names = splitString(talentInfo[1], ",");
            t->name = names[0];
            t->descriptions = splitString(talentInfo[2], ",");
            t->type = static_cast<TalentType>(std::stoi(talentInfo[3]));
            if (t->type == TalentType::SWITCH) {
                if (names.size() <= 1)
                    t->nameSwitch = "Undefined switch name";
                else
                    t->nameSwitch = names[1];
            }
            t->row = std::stoi(talentInfo[4]);
            t->column = std::stoi(talentInfo[5]);
            t->points = std::stoi(talentInfo[6]);
            t->maxPoints = std::stoi(talentInfo[7]);
            t->pointsRequired = std::stoi(talentInfo[8]);
            t->talentSwitch = t->type == TalentType::SWITCH ? std::stoi(talentInfo[9]): 0;
            for (auto& parent : splitString(talentInfo[10], ",")) {
                if (parent == "")
                    break;
                int parentIndex = std::stoi(parent);
                if (talentTree.count(parentIndex)) {
                    addParent(t, talentTree[parentIndex]);
                }
                else {
                    std::shared_ptr<Talent> parentTalent = std::make_shared<Talent>();
                    parentTalent->index = parentIndex;
                    talentTree[parentIndex] = parentTalent;
                    addParent(t, parentTalent);
                }
            }
            for (auto& child : splitString(talentInfo[11], ",")) {
                if (child == "")
                    break;
                int childIndex = std::stoi(child);
                if (talentTree.count(childIndex)) {
                    addChild(t, talentTree[childIndex]);
                }
                else {
                    std::shared_ptr<Talent> childTalent = std::make_shared<Talent>();
                    childTalent->index = childIndex;
                    talentTree[childIndex] = childTalent;
                    addChild(t, childTalent);
                }
            }
            if (t->parents.size() == 0) {
                roots.push_back(t);
            }
        }

        
        tree.talentRoots = roots;

        updateNodeCountAndMaxTalentPointsAndMaxID(tree);
        updateOrderedTalentList(tree);

        return tree;
    }

    /*
    Helper function that splits a string given the delimiter, if string does not contain delimiter then whole string is returned.
    */
    std::vector<std::string> splitString(std::string s, std::string delimiter) {
        std::vector<std::string> stringSplit;

        size_t pos = 0;
        std::string token;
        while ((pos = s.find(delimiter)) != std::string::npos) {
            token = s.substr(0, pos);
            stringSplit.push_back(token);
            s.erase(0, pos + delimiter.length());
        }
        stringSplit.push_back(s);

        return stringSplit;
    }


    /*
    Visualizes a given tree with graphviz. Needs to be installed and the paths have to exist. Generally not safe to use without careful skimming through it.
    */
    void visualizeTree(TalentTree tree, std::string suffix) {
        std::cout << "Visualize tree " << tree.name << " " << suffix << std::endl;
        std::string directory = "C:\\Users\\Tobi\\Documents\\Programming\\CodeSnippets\\WowTalentTrees\\TreesInputsOutputs";

        std::stringstream output;
        output << "strict digraph " << tree.name << " {\n";
        output << "label=\"" + tree.name + "\"\n";
        output << "fontname=\"Arial\"\n";
        //define each node individually
        output << "{\n";
        output << "node [fontname=\"Arial\" width=1 height=1 fixedsize=shape style=filled]\n";
        output << visualizeTalentInformation(tree);
        output << "}\n";
        //define node connections
        for (auto& root : tree.talentRoots) {
            std::stringstream connections;
            visualizeTalentConnections(root, connections);
            output << connections.str();
        }
        output << "}";

        //output txt file in graphviz format
        std::ofstream f;
        f.open(directory + "\\tree_" + tree.name + suffix + ".txt");
        f << output.str();
        f.close();

        std::string command = "\"\"C:\\Program Files\\Graphviz\\bin\\dot.exe\" -Tpng \"" + directory + "\\tree_" + tree.name + suffix + ".txt\" -o \"" + directory + "\\tree_" + tree.name + suffix + ".png\"\"";
        system(command.c_str());
    }

    /*
    Helper function that gathers all individual graphviz talent visualization strings in graphviz language into one string.
    */
    std::string visualizeTalentInformation(TalentTree tree) {
        std::unordered_map<int, std::string> talentInfos;

        for (auto& root : tree.talentRoots) {
            getTalentInfos(root, talentInfos);
        }

        std::stringstream talentInfosStream;
        for (auto& talent : talentInfos) {
            talentInfosStream << talent.first << " " << talent.second << std::endl;
        }

        return talentInfosStream.str();
    }

    /*
    Helper function that recursively gets the specific graphviz visualization string for a talent and its children.
    */
    void getTalentInfos(std::shared_ptr<Talent> talent, std::unordered_map<int, std::string>& talentInfos) {
        talentInfos[talent->index] = "[label=\"" + std::to_string(talent->index) + " " + std::to_string(talent->points) + "/" + std::to_string(talent->maxPoints) + getSwitchLabel(talent) + "\" fillcolor=" + getFillColor(talent) + " shape=" + getShape(talent->type) + "]";
        for (auto& child : talent->children) {
            getTalentInfos(child, talentInfos);
        }
    }

    /*
    Helper function that returns the appropriate fill color for different types of talents and points allocations.
    */
    std::string getFillColor(std::shared_ptr<Talent> talent) {
        if (talent->points == 0) {
            return "white";
        }
        else if (talent->type != TalentType::SWITCH) {
            if (talent->points == talent->maxPoints) {
                return "darkgoldenrod2";
            }
            else {
                return "chartreuse3";
            }
        }
        else if (talent->talentSwitch == 0)
            return "aquamarine3";
        else if (talent->talentSwitch == 1)
            return "coral";
        else
            return "fuchsia";

    }

    /*
    Helper function that defines the shape of each talent type.
    */
    std::string getShape(TalentType type) {
        switch (type) {
        case TalentType::ACTIVE: return "square";
        case TalentType::PASSIVE: return "circle";
        case TalentType::SWITCH: return "octagon";
        }
    }

    /*
    Helper function that displays the switch state of a given talent.
    */
    std::string getSwitchLabel(std::shared_ptr<Talent> talent) {
        if (talent->type == TalentType::SWITCH)
            return "";
        if (talent->talentSwitch == 0)
            return "\nleft";
        else if (talent->talentSwitch == 1)
            return "\nright";
        else
            return "";
    }

    /*
    Helper function that recursively creates the graphviz visualization string of all talent connections. (We only need connections from parents to children.
    Also, graph is defined as strict digraph so we don't need to take care of duplicate arrows.
    */
    void visualizeTalentConnections(std::shared_ptr<Talent> root, std::stringstream& connections) {
        if (root->children.size() == 0)
            return;
        connections << root->index << " -> {";
        for (auto& child : root->children) {
            connections << child->index << " ";
        }
        connections << "}\n";
        for (auto& child : root->children) {
            visualizeTalentConnections(child, connections);
        }
    }

    /*
    Automatically decides the row,col position of tree nodes and reindexes the tree
    */
    void autoPositionTreeNodes(TalentTree& tree) {
        //TTMTODO: Completely rework this system or maybe don't include it at all
        //find depth for each talent
        //depth is the longest path from a root talent that reaches the given talent
        std::unordered_map<std::shared_ptr<Talent>, int> maxDepthMap;
        for (auto& root : tree.talentRoots) {
            findDepthRecursively(0, root, maxDepthMap);
        }

        //create a map that holds all talent per depth value
        std::map<int, std::vector<std::shared_ptr<Talent>>> talentDepths;
        for (auto& talentDepthPair : maxDepthMap) {
            talentDepths[talentDepthPair.second].push_back(talentDepthPair.first);
        }

        //from now on treat depth not as connection length from root but as a row index
        //move talents with high point requirements further down in rows
        bool ordered = false;
        while (!ordered) {
            ordered = true;
            for (auto& depthTalentsPair : talentDepths) {
                //talents at the bottom can't move further down
                if (!talentDepths.count(depthTalentsPair.first + 1))
                    break;
                for (auto& talent : depthTalentsPair.second) {
                    for (auto& lowerTalent : talentDepths[depthTalentsPair.first + 1]) {
                        if (talent->pointsRequired > lowerTalent->pointsRequired) {
                            ordered = false;
                            break;
                        }
                    }
                    if (!ordered) {
                        //move talent one row further down
                        depthTalentsPair.second.erase(std::remove(depthTalentsPair.second.begin(), depthTalentsPair.second.end(), talent), depthTalentsPair.second.end());
                        talentDepths[depthTalentsPair.first + 1].push_back(talent);
                        break;
                    }
                }
                if (!ordered)
                    break;
            }
        }


        //now start the positioning
        //first set up the max width
        int maxWidth = 0;
        for (auto& depthTalentsPair : talentDepths) {
            if (depthTalentsPair.second.size() > maxWidth)
                maxWidth = depthTalentsPair.second.size();
        }
        maxWidth += 4;
        //set up position grid
        std::vector<std::vector<bool>> talentGrid;
        talentGrid.resize(talentDepths.size(), std::vector<bool>(maxWidth));
        std::vector<std::vector<int>> edges;
        std::vector<std::shared_ptr<Talent>> placedTalents;
        int maxCol = -1;
        //iterate through every talent
        for (auto& depthTalentsPair : talentDepths) {
            for (auto& talent : depthTalentsPair.second) {
                float minScore = FLT_MAX;
                int minCol = -1;
                for (int col = 0; col < maxWidth; col++) {
                    if (talentGrid[depthTalentsPair.first][col])
                        continue;
                    talent->row = depthTalentsPair.first;
                    talent->column = col;
                    //cross penalty
                    //TTMTODO: Not sure if this can actually improve anything since it would have to move to the left or move earlier talents
                    float score = checkForCrossing(talent, edges, placedTalents);
                    //distance to parents penalty
                    for (auto& parent : talent->parents) {
                        score += sqDistToParents(talent);
                    }
                    //select minimal penalty
                    if (score < minScore) {
                        minScore = score;
                        minCol = col;
                    }
                }
                talentGrid[depthTalentsPair.first][minCol] = 1;
                talent->row = depthTalentsPair.first;
                talent->column = minCol;
                maxCol = minCol > maxCol ? minCol : maxCol;
                placedTalents.push_back(talent);
                for (auto& parent : talent->parents) {
                    edges.push_back(std::vector<int>{parent->row,parent->column,talent->row,talent->column});
                }
            }
        }

        //make tree symmetric
        int midPointIndex = (maxCol - 1) / 2;
        for (auto& depthTalentsPair : talentDepths) {
            int talentsInRow = depthTalentsPair.second.size();
            for (int i = 0; i < talentsInRow; i++) {
                std::shared_ptr<Talent> talent = depthTalentsPair.second[i];
                int middleTalentIndex = (talentsInRow - 1) / 2; // or no decrement on talentsInRow
                talent->column = midPointIndex - (middleTalentIndex - i);
            }
        }
    }

    float checkForCrossing(std::shared_ptr<Talent> talent, std::vector<std::vector<int>> edges, std::vector<std::shared_ptr<Talent>> placedTalents) {
        return 0;
    }

    float sqDistToParents(std::shared_ptr<Talent> talent) {
        float dist = 0.0f;
        for (auto& parent : talent->parents) {
            dist += (talent->row - parent->row) * (talent->row - parent->row) + (talent->column - parent->column) * (talent->column - parent->column);
        }
        return dist;
    }

    void findDepthRecursively(int depth, std::shared_ptr<Talent> talent, std::unordered_map<std::shared_ptr<Talent>, int>& maxDepthMap) {
        if(depth > maxDepthMap[talent])
            maxDepthMap[talent] = depth;
        for (auto& child : talent->children) {
            findDepthRecursively(depth + 1, child, maxDepthMap);
        }
    }

    /*
    Transforms tree with "complex" talents (that can hold mutliple skill points) to "simple" tree with only talents that can hold a single talent point
    */
    void expandTreeTalents(TalentTree& tree) {
        for (auto& root : tree.talentRoots) {
            expandTalentAndAdvance(root);
        }
        updateNodeCountAndMaxTalentPointsAndMaxID(tree);
        updateOrderedTalentList(tree);
    }

    /*
    Creates all the necessary single point talents to replace a multi point talent and inserts them with correct parents/children
    */
    void expandTalentAndAdvance(std::shared_ptr<Talent> talent) {
        if (talent->maxPoints > 1) {
            std::vector<std::shared_ptr<Talent>> talentParts;
            std::vector<std::shared_ptr<Talent>> originalChildren;
            for (auto& child : talent->children) {
                originalChildren.push_back(child);
            }
            talent->children.clear();
            talentParts.push_back(talent);
            for (int i = 0; i < talent->maxPoints - 1; i++) {
                std::shared_ptr<Talent> t = std::make_shared<Talent>();
                t->index = talent->index;
                t->expansionIndex = i + 1;
                t->isExpanded = true;
                t->name = talent->name + "_" + std::to_string(i + 1);
                t->type = talent->type;
                t->points = 0;
                t->maxPoints = 1;
                t->talentSwitch = talent->talentSwitch;
                t->parents.push_back(talentParts[i]);
                talentParts.push_back(t);
                talentParts[i]->children.push_back(t);
            }
            talent->isExpanded = true;
            for (auto& child : originalChildren) {
                talentParts[talent->maxPoints - 1]->children = originalChildren;
            }
            for (auto& child : originalChildren) {
                std::vector<std::shared_ptr<Talent>>::iterator i = std::find(child->parents.begin(), child->parents.end(), talent);
                if (i != child->parents.end()) {
                    (*i) = talentParts[talent->maxPoints - 1];
                }
                else {
                    //If this happens then n has m as child but m does not have n as parent! Bug!
                    throw std::logic_error("child has missing parent");
                }
            }
            talent->maxPoints = 1;
            for (auto& child : originalChildren) {
                expandTalentAndAdvance(child);
            }
        }
        else {
            for (auto& child : talent->children) {
                expandTalentAndAdvance(child);
            }
        }
    }

    /*
    Transforms tree with "simple" talents (that can only hold one skill point) to "complex" tree with multi-skill-point talents
    */
    void contractTreeTalents(TalentTree& tree) {
        for (auto& root : tree.talentRoots) {
            contractTalentAndAdvance(root);
        }
        updateNodeCountAndMaxTalentPointsAndMaxID(tree);
        updateOrderedTalentList(tree);
    }

    /*
    Creates all the necessary multi point talents to replace a single point talent and inserts them with correct parents/children
    */
    void contractTalentAndAdvance(std::shared_ptr<Talent>& talent) {
        //std::vector<std::string> splitIndex = splitString(talent->index, "_");
        if (talent->isExpanded) {
            //std::vector<std::string> splitName = splitString(talent->name, "_");
            //talent has to be contracted
            //expanded Talents have 1 child at most and talent chain is at least 2 talents long
            //std::string baseIndex = splitIndex[0];
            std::vector<std::shared_ptr<Talent>> talentParts;
            std::shared_ptr<Talent> currTalent = talent;
            talentParts.push_back(talent);
            std::shared_ptr<Talent> childTalent = talent->children[0];
            while (childTalent->index == talent->index) {
                talentParts.push_back(childTalent);
                currTalent = childTalent;
                if (currTalent->children.size() == 0)
                    break;
                childTalent = currTalent->children[0];
            }
            std::shared_ptr<Talent> t = std::make_shared<Talent>();
            t->index = talent->index;
            t->name = talent->name;
            t->type = talent->type;
            t->points = 0;
            for (auto& talent : talentParts) {
                t->points += talent->points;
            }
            t->maxPoints = talentParts.size();
            t->talentSwitch = talent->talentSwitch;
            t->parents = talentParts[0]->parents;
            t->children = talentParts[talentParts.size() - 1]->children;

            //replace talent pointer
            talent = t;
            //iterate through children
            for (auto& child : talent->children) {
                contractTalentAndAdvance(child);
            }
        }
        else {
            for (auto& child : talent->children) {
                contractTalentAndAdvance(child);
            }
        }
    }

}
