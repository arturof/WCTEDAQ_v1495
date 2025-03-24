#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <string.h>
#include <vector>
#include <getopt.h>
#include <pthread.h>

#include <caen++/v1495.hpp>

#include "common.hpp"
#include "jsoncpp/json/value.h"
#include "jsoncpp/json/json.h"

#include <DAQInterface.h>

using namespace ToolFramework;
using namespace std;

//Run in mode that doesn't use spill end alert
bool no_alert_mode = false;

//Types and structure for storing counter information
enum CounterType{ INPUT, LEVEL1, LEVEL2 };
struct CounterInfo{
  CounterType type;
  int index;
  uint16_t address;
  string name;
  bool log;
  uint32_t count;
};

//Function to get counter names from the json file
void getCounterNames(vector<CounterInfo> &counterInfo);

bool keep_running = true;
bool reset_counters = false;
bool save_counters = false;
bool save_counters_cl = false;
static void* userInput_thread(void*){
  while(keep_running){
    char inputC[3];
    std::cin.get(inputC,3);
    if( strcmp(inputC,"q")==0 ) {
      std::cout << "Quitting" << std::endl;
      keep_running = false;
    } else if( strcmp(inputC,"r")==0 ) {
      reset_counters = true;
    } else if( strcmp(inputC,"s")==0 ) {
      save_counters_cl = true;
    } else {
      std::cout << "Wrong Option" << std::endl;
      std::cout << ">";
      std::cout << std::flush;
    }
    std::cin.ignore();
    std::cin.clear();
  }
  return 0;
};
  

// class for automating functions from slowcontrol
class AutomatedFunctions {
  
  public:
  
  AutomatedFunctions(DAQInterface* in_DAQ_inter){
    
    DAQ_inter=in_DAQ_inter;
    
  };
  
  DAQInterface* DAQ_inter;
  
  void end_spill_func(const char* alert){
    
    //DAQ_inter->SdendLog("Hello i received a new_event alert");
    //std::cout <<"Recevied an end of spill alert" <<std::endl;
    save_counters = true;
    //do new event alert action, e.g. reload configuration from DB
    
  }
  
  private:
  
};

void usage(const char* argv0) {
  std::cout
    << "This program reads counter registers from a V1495\n"
       "Usage: " << argv0 << " [options]\n"
       "Allowed options:\n"
       "  --help or -h:              print this message.\n"
       "  --link or -l <string>:     CAEN connection link type. Pass `list' to see supported links.\n"
       "  --arg or -a <number>:      CAEN connection argument. USB device number for USB links, A4818 PID (see on the back) for A4818 links, IP address for ethernet links.\n"
       "  --conet or -c <number>:    CAEN Conet connection daisy chain number\n"
       "  --vme or -v <hexadecimal>: 16 most significant bits of the VME address (the value set by the rotary switches on the board).\n"
       "  --reset or -r:             reset the counters after reading\n"
       "  --noalert or -n:           run in the mode that requires no end of spill alert\n"
       "The output is two columns: the name and the value of a counter\n"
  ;
};

struct Counter {
  const char* name;
  const char* description;
  uint16_t address;
};

#include "counters.hpp"

int main(int argc, char** argv) {

  //Read in the counter register information
  vector<CounterInfo> allCounters;
  ifstream in("register_map.txt");
  string line;
  while(getline(in, line)){
    istringstream iss(line);
    string substring;
    int siter = 0;
    CounterInfo currentLine;
    //Get the values from the comma separated line
    while (getline(iss,substring,',')){
       if(siter==0){
         if(substring.compare("Input")==0) currentLine.type = INPUT;
         if(substring.compare("Level1")==0) currentLine.type = LEVEL1;
         if(substring.compare("Level2")==0) currentLine.type = LEVEL2;
       } else if(siter==1){
         currentLine.index = stoi(substring,0,10);
       } else if(siter==2){
         currentLine.address = uint16_t(stoi(substring,0,16));
       }  
       siter++;
    }
    //Start off with flag to log counter values set to 0
    currentLine.log = false;
    currentLine.name = "";
    currentLine.count = 0;
    allCounters.push_back(currentLine);
  }   

  //Set up the keyboard interrupt
  pthread_t tId;
  (void) pthread_create(&tId, 0, userInput_thread, 0);

  //for(int i=0; i<allCounters.size(); i++) 
  //  cout << allCounters[i].type << " " << allCounters[i].index << " " << hex << allCounters[i].address << " "
  //       << allCounters[i].name << " " << allCounters[i].log << std::endl;


  try {
    caen::Device::Connection connection {
      .link  = CAENComm_USB,
      .arg   = 0,
      .conet = 0,
      .vme   = 0
    };

    const char* arg   = nullptr;
    bool        reset = false;

    while (true) {
      static option options[] = {
        { "arg",     required_argument, nullptr, 'a' },
        { "conet",   required_argument, nullptr, 'c' },
        { "help",    no_argument,       nullptr, 'h' },
        { "link",    required_argument, nullptr, 'l' },
        { "reset",   no_argument,       nullptr, 'r' },
        { "vme",     required_argument, nullptr, 'v' },
        { "noalert", no_argument,       nullptr, 'n' }
      };

      int c = getopt_long(argc, argv, "a:c:hl:rnv:", options, nullptr);
      if (c == -1) break;
      
      switch (c) {
        case 'a':
          arg = optarg;
          break;
        case 'c':
          connection.conet = str_to_uint32(optarg);
          break;
        case 'h':
          usage(argv[0]);
          return 0;
        case 'l':
          if (strcmp(optarg, "list") == 0) {
            list_caen_links();
            return 0;
          };
          connection.link = str_to_link(optarg);
          break;
        case 'r':
          std::cout << "Counter reset after reading is enables" << std::endl;
          reset = true;
          break;
        case 'v':
          connection.vme = str_to_uint32(optarg, 16) << 16;
          break;
        case 'n':
          std::cout << "Using mode with no end-of-spill alert" << std::endl;
          no_alert_mode = true;
          break;
        case '?':
          return 1;
      };
    };


    if (arg)
      if (connection.is_ethernet())
        connection.ip = arg;
      else
        connection.arg = str_to_uint32(arg);

    //caen::V1495 v1495(connection);     

    int verbose = 1;
    std::string Interface_configfile = "./InterfaceConfig";
  
    std::cout<<"Constructing DAQInterface"<<std::endl;
    DAQInterface DAQ_inter(Interface_configfile);
    std::string device_name = DAQ_inter.GetDeviceName(); //name of my device

    std::cout<<"Constructing an AutomatedFunctions helper class to encapsulate callback functions"<<std::endl;
    AutomatedFunctions automated_functions(&DAQ_inter);

    std::cout<<"Updating service status to 'Initialising'"<<std::endl;
    DAQ_inter.sc_vars["Status"]->SetValue("Initialising"); //setting status message

    DAQ_inter.AlertSubscribe("SpillEnd",  std::bind(&AutomatedFunctions::end_spill_func, automated_functions,  std::placeholders::_1));

    std::cout<<"Updating service status to 'Ready'"<<std::endl;
    DAQ_inter.sc_vars["Status"]->SetValue("Ready");
    
    //bool running=true;
    
    std::cout<<"Beginning outer program loop"<<std::endl;
    std::cout<<"To issue a command, press one of the following keys followed by 'Enter':"<<std::endl;
    std::cout<<" 'q' to quit the program"<<std::endl;
    std::cout<<" 'r' to reset the counters"<<std::endl;
    std::cout<<" 's' to save the counters"<<std::endl;
    std::cout<<">";
    std::cout << std::flush;
    int loopCounter = 0;
    bool spill_started = false;
    while(keep_running){ /// outer program loop, runs until user clicks 'Quit' slow control

      //reset counters
      if(reset_counters){
        caen::V1495 v1495(connection);
        v1495.write32(0x3002, 1);
        reset_counters = false;
        std::cout << "Counters Reset" << std::endl;
        std::cout<<"To issue a command, press one of the following keys followed by 'Enter':"<<std::endl;
        std::cout<<" 'q' to quit the program"<<std::endl;
        std::cout<<" 'r' to reset the counters"<<std::endl;
        std::cout<<" 's' to save the counters"<<std::endl;
        std::cout << ">";
        std::cout << std::flush;
      }
      
      //Only execute if save_counters variable has been set to true
      if((save_counters && !no_alert_mode) || save_counters_cl || no_alert_mode){ 

          //Get the counter names and confirm which should be read
          //std::cout << "Get the counter names" << std::endl;
          getCounterNames(allCounters);

          //Make an instance of the monitoring data storage
          Store monitoring_data;

          // read the counters with the log flag set to ttrue and fill the monnitoring data
          caen::V1495 v1495(connection);
          bool counts_match = true;
          for (int i = 0; i<allCounters.size(); i++){
            if(allCounters[i].log){
              uint32_t prev_count = allCounters[i].count;
              allCounters[i].count = v1495.read32(allCounters[i].address);
              if(prev_count != allCounters[i].count) {
                //std::cout << "No Match " << prev_count << " " << allCounters[i].count << " " << allCounters[i].name << std::endl;
                spill_started = true;
                counts_match = false;
              }  
            }  
          }

          if((!no_alert_mode && save_counters) || (no_alert_mode && spill_started && counts_match) || save_counters_cl){
            for (int i = 0; i<allCounters.size(); i++){
              if(allCounters[i].log) monitoring_data.Set(allCounters[i].name, (int)(allCounters[i].count));
              allCounters[i].count = 0;
            }  
              
            monitoring_data.Print();
    
            //Reset the counters
            if (reset) v1495.write32(0x3002, 1);
        
            // convert to JSON
            std::string monitoring_json="";
            monitoring_data>>monitoring_json; /// prducing monitoring json 
        
            // send to the database
            DAQ_inter.SendMonitoringData(monitoring_json);

            save_counters = false;
            save_counters_cl = false;
            spill_started = false;
            std::cout << "Counters Saved" << std::endl;
            std::cout<<"To issue a command, press one of the following keys followed by 'Enter':"<<std::endl;
            std::cout<<" 'q' to quit the program"<<std::endl;
            std::cout<<" 'r' to reset the counters"<<std::endl;
            std::cout<<" 's' to save the counters"<<std::endl;
            std::cout << ">";
            std::cout << std::flush;
          }   
      }
        
      //loopCounter++;
      if(no_alert_mode) usleep(1000000);
      else usleep(3000);
      
    } // end of program loop
    
    (void) pthread_join(tId, NULL);    
    std::cout<<"Application terminated"<<std::endl;
  
/*
*/

    return 0;
  } catch (std::exception& e) {
    std::cerr << argv[0] << ": " << e.what() << std::endl;
  };

};

void getCounterNames(vector<CounterInfo> &counterInfo){

  //reset values
  for(int i=0; i<counterInfo.size(); i++){
    counterInfo[i].log = false;
    counterInfo[i].name = "";
  }   
  //Get names corresponding to addresses from the json file
  Json::Value counters;
  std::ifstream counter_file("/home/mpmt/firmware/TriggerConfig/configurations/written_config.json",std::ifstream::binary);
  counter_file >> counters;

  //Iterate through the addresses and check for associated names
  //If no entry in json file, we set the logging variable to false
  for(int i=0; i<counterInfo.size(); i++){
    string counterType = "input_signals";
    if(counterInfo[i].type == LEVEL1) counterType = "level_1_logics";
    else if(counterInfo[i].type == LEVEL2) counterType = "level_2_logics";
    counterInfo[i].name = counters[counterType][to_string(counterInfo[i].index)]["short_name"].asString();
    if(counterInfo[i].name.empty()){
      counterInfo[i].log = false;
      counterInfo[i].name = "";
    } else counterInfo[i].log = true;
    //cout << counterInfo[i].type << " " << counterInfo[i].index << " " << hex << counterInfo[i].address << " "
    //     << counterInfo[i].name << " " << counterInfo[i].log << std::endl;
  }  

}
