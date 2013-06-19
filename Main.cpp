//-----------------------------------------------------------------------------
//
//	Main.cpp
//
//	Minimal application to test OpenZWave.
//
//	Creates an OpenZWave::Driver and the waits.  In Debug builds
//	you should see verbose logging to the console, which will
//	indicate that communications with the Z-Wave network are working.
//
//	Copyright (c) 2010 Mal Lansell <mal@openzwave.com>
//
//
//	SOFTWARE NOTICE AND LICENSE
//
//	This file is part of OpenZWave.
//
//	OpenZWave is free software: you can redistribute it and/or modify
//	it under the terms of the GNU Lesser General Public License as published
//	by the Free Software Foundation, either version 3 of the License,
//	or (at your option) any later version.
//
//	OpenZWave is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public License
//	along with OpenZWave.  If not, see <http://www.gnu.org/licenses/>.
//
//-----------------------------------------------------------------------------
// Other modifications completed by conradvassallo.com, then thomasloughlin.com, then nick@linkstudios.co.uk, then gd@geoffroydoucet.com

#include <unistd.h>
#include <pthread.h>
#include "Options.h"
#include "Manager.h"
#include "Driver.h"
#include "Node.h"
#include "Group.h"
#include "Notification.h"
#include "ValueStore.h"
#include "Value.h"
#include "ValueBool.h"

#include "md5.h"
#include "ServerSocket.h"
#include "SocketException.h"
#include <string>
#include <iostream>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <sstream>
#include <iostream>
#include <libconfig.h>
#include <algorithm>
#include <cctype>

using namespace OpenZWave;

static uint32 g_homeId = 0;
static bool g_initFailed = false;


const static char *password = "changeme";

typedef struct {
    uint32 m_homeId;
    uint8 m_nodeId;
    bool m_polled;
    list<ValueID> m_values;
} NodeInfo;

typedef struct {
  ServerSocket* socket;
} Param;

// Value-Defintions of the different String values

static list<NodeInfo*> g_nodes;
static pthread_mutex_t g_criticalSection;
static pthread_cond_t initCond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t initMutex = PTHREAD_MUTEX_INITIALIZER;

//-----------------------------------------------------------------------------
// <GetNodeInfo>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------

NodeInfo* GetNodeInfo(Notification const* _notification) {
    uint32 const homeId = _notification->GetHomeId();
    uint8 const nodeId = _notification->GetNodeId();
    for (list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it) {
        NodeInfo* nodeInfo = *it;
        if ((nodeInfo->m_homeId == homeId) && (nodeInfo->m_nodeId == nodeId)) {
            return nodeInfo;
        }
    }
    
    return NULL;
}

//-----------------------------------------------------------------------------
// <OnNotification>
// Callback that is triggered when a value, group or node changes
//-----------------------------------------------------------------------------

void OnNotification(Notification const* _notification, void* _context) {
    // Must do this inside a critical section to avoid conflicts with the main thread
    pthread_mutex_lock(&g_criticalSection);
    
    switch (_notification->GetType()) {
        case Notification::Type_ValueAdded:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                // Add the new value to our list
                nodeInfo->m_values.push_back(_notification->GetValueID());
            }
            break;
        }
            
        case Notification::Type_ValueRemoved:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                // Remove the value from out list
                for (list<ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it) {
                    if ((*it) == _notification->GetValueID()) {
                        nodeInfo->m_values.erase(it);
                        break;
                    }
                }
            }
            break;
        }
            
        case Notification::Type_ValueChanged:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                // One of the node values has changed
                // TBD... //GetValueID()
                //nodeInfo = nodeInfo;
                for (list<ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it) {
                    if ((*it) == _notification->GetValueID()) {
                        nodeInfo->m_values.erase(it);
                        break;
                    }
                }
                nodeInfo->m_values.push_back(_notification->GetValueID());
                //Todo: clean up this update.  This was a fast way to update the status
            }
            break;
        }
            
        case Notification::Type_Group:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                // One of the node's association groups has changed
                // TBD...
                nodeInfo = nodeInfo;
            }
            break;
        }
            
        case Notification::Type_NodeAdded:
        {
            // Add the new node to our list
            NodeInfo* nodeInfo = new NodeInfo();
            nodeInfo->m_homeId = _notification->GetHomeId();
            nodeInfo->m_nodeId = _notification->GetNodeId();
            nodeInfo->m_polled = false;
            g_nodes.push_back(nodeInfo);
            break;
        }
            
        case Notification::Type_NodeRemoved:
        {
            // Remove the node from our list
            uint32 const homeId = _notification->GetHomeId();
            uint8 const nodeId = _notification->GetNodeId();
            for (list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it) {
                NodeInfo* nodeInfo = *it;
                if ((nodeInfo->m_homeId == homeId) && (nodeInfo->m_nodeId == nodeId)   ) {
                    g_nodes.erase(it);
                    break;
                }
            }
            break;
        }
            
        case Notification::Type_NodeEvent:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                // We have received an event from the node, caused by a
                // basic_set or hail message.
                // TBD...
                nodeInfo = nodeInfo;
            }
            break;
        }
            
        case Notification::Type_PollingDisabled:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                nodeInfo->m_polled = false;
            }
            break;
        }
            
        case Notification::Type_PollingEnabled:
        {
            if (NodeInfo * nodeInfo = GetNodeInfo(_notification)) {
                nodeInfo->m_polled = true;
            }
            break;
        }
            
        case Notification::Type_DriverReady:
        {
            g_homeId = _notification->GetHomeId();
            break;
        }
            
            
        case Notification::Type_DriverFailed:
        {
            g_initFailed = true;
            pthread_cond_broadcast(&initCond);
            break;
        }
            
        case Notification::Type_AwakeNodesQueried:
        case Notification::Type_AllNodesQueried:
        {
            pthread_cond_broadcast(&initCond);
            break;
        }
            
        default:
        {
        }
    }
    
    pthread_mutex_unlock(&g_criticalSection);
}

/******** DOSTUFF() *********************
 There is a separate instance of this function
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void split(const string& s, char c, vector<string>& v) {
    string::size_type i = 0;
    string::size_type j = s.find(c);
    while (j != string::npos) {
        v.push_back(s.substr(i, j - i));
        i = ++j;
        j = s.find(c, j);
        if (j == string::npos)
            v.push_back(s.substr(i, s.length()));
    }
}

string trim(string s) {
    return s.erase(s.find_last_not_of(" \n\r\t") + 1);
}


string getNodeType(uint32 homeid, uint8 nodeid) {
	string nodeType = Manager::Get()->GetNodeType(g_homeId, nodeid);
	
	if ( (nodeType.compare("Multilevel Switch") == 0) || (nodeType.compare("Multilevel Power Switch") == 0) || (nodeType.compare("Multilevel Scene Switch") == 0) ) {
		return "Multilevel Switch";
	} else if ((nodeType.compare("Binary Switch") == 0 ) || (nodeType.compare("Binary Scene Switch") == 0 )) {
		return "Binary Switch";
	} 
	
	return nodeType;
}

ValueID* getLevelValueID(uint32 homeid, uint8 nodeid, string label) {
    NodeInfo* nodeInfo;
    bool nodeinfo_found = false;
	
	string nodeType = getNodeType(g_homeId, nodeid);
	
    for (list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it) {
        NodeInfo* g_nodeInfo = *it;
        if ((g_nodeInfo->m_homeId == homeid) && (g_nodeInfo->m_nodeId == nodeid)) {
            nodeInfo = g_nodeInfo;
            nodeinfo_found=true;
        }
    }
	if( nodeinfo_found) {
    	for (list<ValueID>::iterator it = nodeInfo->m_values.begin(); it != nodeInfo->m_values.end(); ++it) {
        	if( Manager::Get()->GetValueLabel( *it).compare(label) == 0 ) {
				return &(*it);
        	}    
    	}
	}
	
	return NULL;
}

void setNodeLevel(uint32 homeid, uint8 nodeid, uint8 level) {
	
	
	ValueID* valueid;
	string nodeType = getNodeType(homeid, nodeid);
	if( nodeType.compare("Multilevel Switch") == 0) {
		valueid = getLevelValueID(homeid, nodeid, "Level");
		if( valueid ) {
			if (!Manager::Get()->SetValue(*valueid, level))
				printf("Failed set value\n");	
		}
	} else if ( nodeType.compare("Binary Switch") == 0 ) {
		valueid = getLevelValueID(homeid, nodeid, "Switch");
		if( valueid ) {
			bool switch_value;
			if (level == 0) {
				switch_value = false;
			} else {
				switch_value = true;
			}
			if (!Manager::Get()->SetValue(*valueid, switch_value))
				printf("Failed set value\n");	
		}
		
	} else {
		printf("Value ID not found\n");
	}
}


void getNodeLevel(uint32 homeid, NodeInfo* nodeInfo , string & value) {
	
	ValueID* valueid;
	uint8 nodeid = nodeInfo->m_nodeId;
	string nodeType = getNodeType(homeid, nodeid);
	if( nodeType.compare("Multilevel Switch") == 0) {
		valueid = getLevelValueID(homeid, nodeid, "Level");
		if( valueid ) {
			Manager::Get()->GetValueAsString(*valueid, &value);	
		}
	} else if ( nodeType.compare("Binary Switch") == 0 ) {
		valueid = getLevelValueID(homeid, nodeid, "Switch");
		if( valueid ) {
			bool switch_value;
			Manager::Get()->GetValueAsBool(*valueid, &switch_value);
			if(switch_value) {
				value = "99";
			} else {
				value = "0";
			}	
		}
		
	} else {
		printf("unknown type: %s\n", nodeType.c_str());
	}


}

string get_cookie() {
    int cookie = rand() % 65536;
    
    stringstream ss;
    ss << cookie;
    return ss.str(); 
}


void server_send(ServerSocket& socket, string cmd) {
    socket << cmd  + "\r\n";
   
    printf("S: %s\n", cmd.c_str());
}

void server_receive(ServerSocket& socket, string &str) {

    socket >> str;
    printf("C: %s", str.c_str());
}

void send_msg(ServerSocket& socket, const string &str) {
    server_send(socket, "MSG~"+str);
}

void send_err(ServerSocket& socket, const string &str) {
    server_send(socket, "ERR~"+str);
}

void* handle_lightswitch(void *parg) {
    Param *p = (Param *)parg;
    ServerSocket &new_sock = *p->socket;
    try {
        bool verified = false;
        string version = "VER~1.1";
        string passwd_md5;

        server_send(new_sock, "6004 ZwaveCommander Server");
        while (true) {             
            std::string data;
            server_receive(new_sock, data);
            vector<string> args;
            string command;
            split(trim(data), '~', args);
            if(args.size() > 0 ) {
                command = args[0];
            } else {
                command = trim(data);
            }

            if( !verified) {
                if(command.compare("IPHONE") == 0) {
                    string cookie = get_cookie();
                    server_send(new_sock, "COOKIE~"+cookie);
                    server_receive(new_sock, data);
                    passwd_md5 = "PASSWORD~" + md5(cookie+":"+string(password));
                    transform(passwd_md5.begin(), passwd_md5.end(), passwd_md5.begin(), ::toupper);
                    
                    if(trim(data).compare(passwd_md5) == 0) {
                        verified = true;
                    } else {
                        send_err(new_sock, "Passwords Do Not Match!");
                        break;
                    }
                    
                    server_send(new_sock, version);

                } else {
                    send_err(new_sock, "Invalid command " + command);
                    break;  
                }


            } else {
                if(command.compare("VERSION") == 0 || command.compare("SERVER") == 0) {
                    server_send(new_sock, version);
                } else if(command.compare("TERMINATE") == 0) {
                    break;
                } else if(command.compare("ALIST") == 0) {
                    string device;
                    for (list<NodeInfo*>::iterator it = g_nodes.begin(); it != g_nodes.end(); ++it) {
                        NodeInfo* nodeInfo = *it;
                        int nodeID = nodeInfo->m_nodeId;
                        string nodeType = getNodeType(g_homeId, nodeInfo->m_nodeId);
                        string nodeName = Manager::Get()->GetNodeName(g_homeId, nodeInfo->m_nodeId);
                        string nodeZone = Manager::Get()->GetNodeLocation(g_homeId, nodeInfo->m_nodeId);
                        string nodeValue ="";
                        

                        if (nodeName.size() == 0) 
                            nodeName = "Undefined";
                               
                        if (nodeType != "Static PC Controller") {
                            getNodeLevel(g_homeId, nodeInfo, nodeValue);
                            stringstream ssNodeName, ssNodeId, ssNodeType, ssNodeZone, ssNodeValue;
                            ssNodeName << nodeName;
                            ssNodeId << nodeID;
                            ssNodeType << nodeType;
                            ssNodeZone << nodeZone;
                            ssNodeValue << nodeValue;
                            device = "DEVICE~" + ssNodeName.str() + "~" + ssNodeId.str() + "~"+  ssNodeValue.str() +"~" + ssNodeType.str();
                                
                            server_send(new_sock, device);
                        }
                    }
                    server_send(new_sock, "ENDLIST");
                        
                } else if (command.compare("SLIST") == 0) {
                        server_send(new_sock, "ENDLIST");
                } else if (command.compare("ZLIST") == 0) {
                        server_send(new_sock, "ENDLIST");
                } else if (command.compare("HEALNETWORK") == 0) {
                        Manager::Get()->HealNetwork(g_homeId, true);   
                } else if (command.compare("TESTNETWORK") == 0 ) { 
                        Manager::Get()->TestNetwork(g_homeId, 5); 
                } else if ( (command.compare("DEVICE") == 0 ) && ( args.size() == 4 ) ){
                        uint32 Node = 0;
                        uint32 Level = 0;
                        string Type = args[3];

                        stringstream ssNumNode(args[1]);
                        stringstream ssNumLevel(args[2]);

                        ssNumNode >> Node;
                        ssNumLevel >> Level;
                                

                        if ( (Type.compare("Multilevel Switch") == 0) || (Type.compare("Multilevel Power Switch") == 0) || (Type.compare("Multilevel Scene Switch") == 0) ) {
                            pthread_mutex_lock(&g_criticalSection);
                            setNodeLevel(g_homeId, (uint8)Node, (uint8)Level);
                            pthread_mutex_unlock(&g_criticalSection);
                        } else if ((Type.compare("Binary Switch") == 0 ) || (Type.compare("Binary Scene Switch") == 0 )) {
                            pthread_mutex_lock(&g_criticalSection);
                            if (Level == 0) {
                                setNodeLevel(g_homeId, (uint8)Node, Level);
                            } else {
                                Level = 255;
                                setNodeLevel(g_homeId, (uint8)Node, Level);
                            }
                            pthread_mutex_unlock(&g_criticalSection);
                                    
                        }

                        send_msg(new_sock, "ZWave Node=" + args[1] + " Level=" + args[2]);

                } else if ( (command.compare("SETNODE") == 0) && (args.size() == 4) ) {
                        
                        uint32 Node = 0;
                        string NodeName = "";
                        string NodeZone = "";
                                
                        stringstream ssNumNode(args[1]);

                        ssNumNode >> Node;
                        
                        NodeName = args[2];
                        NodeZone = args[3];
                                
                        pthread_mutex_lock(&g_criticalSection);
                        Manager::Get()->SetNodeName(g_homeId, Node, NodeName);
                        Manager::Get()->SetNodeLocation(g_homeId, Node, NodeZone);
                        pthread_mutex_unlock(&g_criticalSection);
                                
                        stringstream ssNode, ssName, ssZone;
                        ssNode << Node;
                        ssName << NodeName;
                        ssZone << NodeZone;
                        send_msg(new_sock, "ZWave Name set Node=" + ssNode.str() + " Name=" + ssName.str() + " Zone=" + ssZone.str());
                                
                        //save details to XML
                        Manager::Get()->WriteConfig(g_homeId);
                } else {
                    printf("unknown command %s\n", command.c_str());
                }

            }
                                            
        }
    } catch (SocketException&) {
    }
    delete(p->socket);
    free(p);

    return NULL;
}


//-----------------------------------------------------------------------------
// <main>
// Create the driver and then wait
//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    config_t cfg;
    const char *usb = "/dev/ttyUSB0";
    const char *ozwconfigs = "../open-zwave/config/";
    const char *cmd = "";
    int port = 6004;
    
    const char *config_file_name = "server.cfg";

    pthread_t thread;
    
    /*pthread_mutexattr_t mutexattr;
    
    pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&g_criticalSection, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);
    
    pthread_mutex_lock(&initMutex);
    */
    
    /* Initialization */
    config_init(&cfg);
    
    /* Read the file. If there is an error, report it and exit. */
    if (!config_read_file(&cfg, config_file_name)) {
        printf("Error unable to process config file\n\n");
        config_destroy(&cfg);
        return -1;
    }
    
    if (config_lookup_int(&cfg, "port", &port)) {
        printf("\nPort to run server on: %lu", port);
    } else {
        port = 6004;
    }
    
    if (config_lookup_string(&cfg, "usb", &usb)) {
        printf("\nUSB port: %s", usb);
    } else {
        cmd = "";
    }
    
    if (config_lookup_string(&cfg, "configs", &ozwconfigs)) {
        printf("\nUsing configs from: %s", ozwconfigs);
    } else {
        cmd = "";
    }
    
    if (config_lookup_string(&cfg, "cmd", &cmd)) {
        printf("\nCommand to execute for events: %s", cmd);
    } else {
        cmd = "";
    }

    if (config_lookup_string(&cfg, "password", &cmd)) {
        printf("\nPassword for the client: %s", password);
    } else {
        password = "changeme";
    }
    
    printf("\n");
    
    
    
    // Create the OpenZWave Manager.
    // The first argument is the path to the config files (where the manufacturer_specific.xml file is located
    // The second argument is the path for saved Z-Wave network state and the log file.  If you leave it NULL
    // the log file will appear in the program's working directory.
    Options::Create(ozwconfigs, "", "");
    Options::Get()->Lock();
    
    Manager::Create();
    
    // Add a callback handler to the manager.  The second argument is a context that
    // is passed to the OnNotification method.  If the OnNotification is a method of
    // a class, the context would usually be a pointer to that class object, to
    // avoid the need for the notification handler to be a static.
    Manager::Get()->AddWatcher(OnNotification, NULL);
    
    // Add a Z-Wave Driver
    Manager::Get()->AddDriver((argc > 1) ? argv[1] : usb);
    //Manager::Get()->AddDriver( "HID Controller", Driver::ControllerInterface_Hid );
    
    // Now we just wait for the driver to become ready, and then write out the loaded config.
    // In a normal app, we would be handling notifications and building a UI for the user.
    
    pthread_cond_wait(&initCond, &initMutex);

    if (!g_initFailed) {
        
        Manager::Get()->WriteConfig(g_homeId);    

        try {
            // Create the socket
            ServerSocket server(port);
            while (true) {
                
                Param *p;
                p=(Param *)malloc(sizeof(Param));
                p->socket  = new ServerSocket;
                server.accept(*p->socket);
                pthread_create(&thread, 0, handle_lightswitch, (void*)p);
            }
        } catch (SocketException& e) {
            printf("Exception was caught:");
        }
    }
    
    Manager::Destroy();
    
    
    pthread_mutex_destroy(&g_criticalSection);
    
    config_destroy(&cfg);
    return 0;
}
