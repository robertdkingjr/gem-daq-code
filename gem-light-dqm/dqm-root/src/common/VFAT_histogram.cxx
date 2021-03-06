#include "Hardware_histogram.h"
#include "TH1.h"
#include <Event.h>
#define NCHANNELS 128

//!A class that creates histogram for VFAT data
class VFAT_histogram: public Hardware_histogram
{
  public:
    //!Calls the base constructor
    VFAT_histogram(const std::string & filename, TDirectory * dir, const std::string & hwid):Hardware_histogram(filename, dir, hwid){}//call base constructor
    //!Calls the base constructor
    VFAT_histogram(VFAT_histogram * vH):Hardware_histogram("dummy", vH->m_dir, vH->m_HWID){}//call base constructor
    ~VFAT_histogram(){}

    //!Books histograms for VFAT data
    /*!
     This books histograms for the following data: Difference between crc and recalculated crc, Control Bit 1010, Control Bit 1100, Control Bit 1110, Bunch Crossing Number, Event Counter, Control Flags, and Chip ID, and Fired Channels. It also creates a subdirectory for Threshold Scans and books those histograms.
     */
    void bookHistograms(){
      m_dir->cd();
      b1010    = new TH1F("b1010", "Control Bits", 15,  0x0 , 0xf);
      BC       = new TH1F("BC", "Bunch Crossing Number", 4095,  0x0 , 0xfff);
      b1100    = new TH1F("b1100", "Control Bits", 15,  0x0 , 0xf);
      EC       = new TH1F("EC", "Event Counter", 255,  0x0 , 0xff);
      Flag     = new TH1F("Flag", "Control Flags", 15,  0x0 , 0xf);
      b1110    = new TH1F("b1110", "Control Bits", 15,  0x0 , 0xf);
      ChipID   = new TH1F("ChipID", "Chip ID", 4095,  0x0 , 0xfff);
      SlotN    = new TH1F("SlotN", "Slot Number", 24,  0, 24);
      FiredChannels   = new TH1F("FiredChannels", "FiredChannels", 128,  0, 128);
      FiredStrips   = new TH1F("FiredStrips", "FiredStrips", 128,  0, 128);
      crc      = new TH1F("crc", "check sum value", 0xffff,  0x0 , 0xffff);
      crc_calc = new TH1F("crc_calc", "check sum value recalculated", 0xffff,  0x0 , 0xffff);
      crc_difference = new TH1F("crc_difference", "difference between crc and crc_calc", 0xffff,  -32768 , 32768);
      latencyScan = new TH1F("latencyScan", "Latency Scan", 255,  0, 255);
      thresholdScanChip = new TH1F("thresholdScan","Threshold Scan",255,0,255);
      TDirectory * scandir = gDirectory->mkdir("Threshold_Scans");
      scandir->cd();
      for (int i = 0; i < 128; i++){
        thresholdScan[i] = new TH1F(("thresholdScan"+to_string(static_cast<long long int>(i))).c_str(),("thresholdScan"+to_string(static_cast<long long int>(i))).c_str(),256,0,256);
      }// end loop on channels
      gDirectory->cd("..");
    }
    
    //!Fills histograms for VFAT data
    /*!
     This fills histograms for the following data: Difference between crc and recalculated crc, Control Bit 1010, Control Bit 1100, Control Bit 1110, Bunch Crossing Number, Event Counter, Control Flags, and Chip ID, and Fired Channels
     */
        void fillHistograms(VFATdata * vfat){
      setVFATBlockWords(vfat); 
      crc_difference->Fill(vfat->crc()-checkCRC(vfatBlockWords));  
      b1010->Fill(vfat->b1010());
      b1100->Fill(vfat->b1100());
      b1110->Fill(vfat->b1110());
      BC->Fill(vfat->BC());
      EC->Fill(vfat->EC());
      Flag->Fill(vfat->Flag());
      ChipID->Fill(vfat->ChipID());
      m_sn = std::stoi(m_HWID);
      crc->Fill(vfat->crc());
      setVFATBlockWords(vfat);
      crc_calc->Fill(checkCRC(vfatBlockWords));
      SlotN->Fill(m_sn);
      this->readMap(m_sn);
      uint16_t chan0xf = 0;
      for (int chan = 0; chan < 128; ++chan) {
        if (chan < 64){
          chan0xf = ((vfat->lsData() >> chan) & 0x1);
          if(chan0xf) {
            FiredChannels->Fill(chan);
            FiredStrips->Fill(m_strip_map[chan]);
          }
        } else {
          chan0xf = ((vfat->msData() >> (chan-64)) & 0x1);
          if(chan0xf) FiredChannels->Fill(chan);
        }
      }
    }
    //!Fills the histograms for the Threshold Scans
    void fillScanHistograms(VFATdata * vfat, int runtype, int deltaV, int latency){
      bool channelFired = false;
      for (int i = 0; i < 128; i++){
        uint16_t chan0xf = 0;
        if (i < 64){
          chan0xf = ((vfat->lsData() >> i) & 0x1);
          if(chan0xf) {
            thresholdScan[i]->Fill(deltaV);
            channelFired = true;
          }
        } else {
          chan0xf = ((vfat->msData() >> (i-64)) & 0x1);
          if(chan0xf) {
            thresholdScan[i]->Fill(deltaV);
            channelFired = true;
          }
        }
        if (channelFired) {
          latencyScan->Fill(latency);
          thresholdScanChip->Fill(deltaV);
        }
      }// end loop on channels
    }
  private:
    TH1F* b1010;            ///<Histogram for control bit 1010
    TH1F* BC;               ///<Histogram for Bunch Crossing Number
    TH1F* b1100;            ///<Histogram for control bit 1100
    TH1F* EC;               ///<Histogram for Event Counter
    TH1F* Flag;             ///<Histogram for Control Flags
    TH1F* b1110;            ///<Histogram for contorl bit 1110
    TH1F* ChipID;           ///<Histogram for Chip ID
    TH1F* FiredChannels;    ///<Histogram for Fired Channels (uses lsData and fmData)
    TH1F* crc_difference;   ///<Histogram for difference of crc and recalculated crc
    TH1F* SlotN;
    TH1F* FiredStrips;
    TH1F* crc;
    TH1F* crc_calc;
    TH1F* latencyScan;
    TH1F* thresholdScanChip;
    TH1F* thresholdScan[NCHANNELS];
    int m_strip_map[128];
    int m_sn;
    void readMap(int sn){
      std::string path = std::getenv("BUILD_HOME");
      if (sn < 2) {
        path += "/gem-light-dqm/dqm-root/data/v2b_schema_chips0-1.csv";
      } else if (sn < 16) {
        path += "/gem-light-dqm/dqm-root/data/v2b_schema_chips2-15.csv";
      } else if (sn < 18) {
        path += "/gem-light-dqm/dqm-root/data/v2b_schema_chips16-17.csv";
      } else {
        path += "/gem-light-dqm/dqm-root/data/v2b_schema_chips18-23.csv";
      }
      this->readCSV(path);
    }
    void readCSV(std::string ifpath_){
      std::ifstream icsvfile_;
      icsvfile_.open(ifpath_);
      if(!icsvfile_.is_open()) {
        std::cout << "\nThe file: " << icsvfile_ << " is missing.\n" << std::endl;
        return;
      }  
      for (int il = 0; il < 128; il++) {
        std::string line;
        std::getline(icsvfile_, line);
        std::istringstream iss(line);
        std::string val;
        std::getline(iss,val,',');
        std::stringstream convertor(val);
        int strip;
        convertor >> std::dec >> strip;
        std::getline(iss,val,',');
        convertor.str("");
        convertor.clear();
        convertor << val;
        int channel;
        convertor >> std::dec >> channel;
        m_strip_map[channel] = strip;
      }
    }

    uint16_t vfatBlockWords[12];   ///<Array of uint16_t used for setVFATBlockWords

    //!This puts the VFAT data in an array of uint16_t to be used for the crc check
    void setVFATBlockWords(VFATdata * vfat_)
    {
      vfatBlockWords[11] = ((0x000f & vfat_->b1010())<<12) |  vfat_->BC();
      vfatBlockWords[10] = ((0x000f & vfat_->b1100())<<12) | vfat_->EC() | ((0x000f & vfat_->Flag())<<12);
      vfatBlockWords[9]  = ((0x000f & vfat_->b1110())<<12) | vfat_->ChipID();
      vfatBlockWords[8]  = (0xffff000000000000 & vfat_->msData()) >> 48;
      vfatBlockWords[7]  = (0x0000ffff00000000 & vfat_->msData()) >> 32;
      vfatBlockWords[6]  = (0x00000000ffff0000 & vfat_->msData()) >> 16;
      vfatBlockWords[5]  = (0x000000000000ffff & vfat_->msData());
      vfatBlockWords[4]  = (0xffff000000000000 & vfat_->lsData()) >> 48;
      vfatBlockWords[3]  = (0x0000ffff00000000 & vfat_->lsData()) >> 32;
      vfatBlockWords[2]  = (0x00000000ffff0000 & vfat_->lsData()) >> 16;
      vfatBlockWords[1]  = (0x000000000000ffff & vfat_->lsData());
     }

    //!Recalculates the CRC to be compared to original CRC. Difference should be 0.
    uint16_t checkCRC(uint16_t dataVFAT[11])
       {
         uint16_t crc_fin = 0xffff;
         for (int i = 11; i >= 1; i--)
         {
           crc_fin = this->crc_cal(crc_fin, dataVFAT[i]);
         }
         return(crc_fin);
       }
    //!Called by checkCRC
    uint16_t crc_cal(uint16_t crc_in, uint16_t dato)
       {
         uint16_t v = 0x0001;
         uint16_t mask = 0x0001;    
         bool d=0;
         uint16_t crc_temp = crc_in;
         unsigned char datalen = 16;
          
         for (int i=0; i<datalen; i++){
           if (dato & v) d = 1;
           else d = 0;
           if ((crc_temp & mask)^d) crc_temp = crc_temp>>1 ^ 0x8408;
           else crc_temp = crc_temp>>1;
           v<<=1;
         }
         return(crc_temp);
       }
};

