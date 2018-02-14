#include <stdio.h>
#include <iostream>
#include <string.h>
#include <iomanip>
#include <vector>
#include <array>
#include <inttypes.h>
#include <boost/shared_ptr.hpp>
#include <cpsw_api_user.h>
#include <yaml-cpp/yaml.h>
#include <getopt.h>
#include <stdexcept>
#include <arpa/inet.h>

#include "l2Mps_bcm.h"

class IYamlSetIP : public IYamlFixup
{
    public:
        IYamlSetIP( std::string ip_addr ) : ip_addr_(ip_addr) {}

        void operator()(YAML::Node &node)
        {
            node["ipAddr"] = ip_addr_.c_str();
        }

        ~IYamlSetIP() {}

    private:
        std::string ip_addr_;
};

int main(int argc, char **argv)
{
    unsigned char buf[sizeof(struct in6_addr)];
    std::string ipAddr;
    std::string yamlDoc;
    int c;

    while((c =  getopt(argc, argv, "a:Y:")) != -1)
    {
        switch (c)
        {
            case 'a':
                if (!inet_pton(AF_INET, optarg, buf))
                {
                    std::cout << "Invalid IP address..." << std::endl;
                    exit(1);
                }
                ipAddr = std::string(optarg);
                break;
            case 'Y':
                yamlDoc = std::string(optarg);
                break;
            default:
                std::cout << "Invalid option. Use -a <IP_address> -Y <Yaml_top>" << std::endl;
                exit(1);
                break;
        }
    }

    if (ipAddr.empty())
    {
        std::cout << "Must specify an IP address." << std::endl;
        exit(1);
    }

    if (yamlDoc.empty())
    {
        std::cout << "Must specify a Yaml top path." << std::endl;
        exit(1);
    }

    const char *mpsRootName = "mmio/AmcCarrierCore/AppMps";

    IYamlSetIP setIP(ipAddr);
    Path root = IPath::loadYamlFile( yamlDoc.c_str(), "NetIODev", NULL, &setIP );    

    Path mpsRoot;
    try
    {
        mpsRoot = root->findByName(mpsRootName);

        std::array<MpsBcm, 2> myMpsBcm;

        for (std::size_t i {0}; i < 2; ++i)
        {
            std::cout << "BCM for AMC[" << i << "]: BCM[" << i << "]" << std::endl;
            std::cout << "====================================================" << std::endl;
            myMpsBcm[i] = MpsBcmFactory::create(mpsRoot, i);
            std::cout << "====================================================" << std::endl;

            std::cout << std::endl;

            int n;
            
            for (int j = 0; j < numBcmChs; ++j)
            {
                try 
                {
                    bcm_channel_t bcmCh = j;

                    std::cout << std::endl;
                    std::cout << "  ===============================================" << std::endl;
                    std::cout << "  Channel = " << j << ":" << std::endl;
                    std::cout << "  ===============================================" << std::endl;

                    std::cout << "    Thr channel =   " << myMpsBcm[i]->getChannel(bcmCh) << std::endl;
                    std::cout << "    Thr count =     " << myMpsBcm[i]->getThrCount(bcmCh) << std::endl;
                    std::cout << "    Byte map =      " << myMpsBcm[i]->getByteMap(bcmCh) << std::endl;
                    std::cout << "    Idle Enabled =  " << std::boolalpha << myMpsBcm[i]->getIdleEn(bcmCh) << std::endl;
                    std::cout << "    Lcls1 Enabled = " << std::boolalpha << myMpsBcm[i]->getLcls1En(bcmCh) << std::endl;
                    std::cout << "    Alt Enabled =   " << std::boolalpha << myMpsBcm[i]->getAltEn(bcmCh) << std::endl;

                    std::cout << std::endl;
                    std::cout << "    Threshold tables:" << std::endl;
                    std::cout << "    ==========================================" << std::endl;
                    std::cout << "    Table     " << "   minEn" << "     min" << "   maxEn" << "     max" << std::endl;
                    std::cout << "    ==========================================" << std::endl;

                    bcmThr_channel_t bcmThrCh;
                    bcmThrCh.bcmCh = bcmCh;
                    
                    try
                    {
                        bcmThrCh.thrTb = thr_table_t{{0,0}};
                        std::cout << "    LCLS1     ";
                        
                        std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMinEn(bcmThrCh);
                        std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMin(bcmThrCh);
                        std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMaxEn(bcmThrCh);
                        std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMax(bcmThrCh);
                        std::cout << std::endl;
                    }
                    catch (std::exception &e)
                    {
                        printf("   Error on LCLS 1 table section: %s", e.what());
                    }

                    try
                    {
                        bcmThrCh.thrTb = thr_table_t{{1,0}};
                        std::cout << "    IDLE      ";
                        
                        std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMinEn(bcmThrCh);
                        std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMin(bcmThrCh);
                        std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMaxEn(bcmThrCh);
                        std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMax(bcmThrCh);
                        std::cout << std::endl;
                    }
                    catch (std::exception &e)
                    {
                        printf("   Error on IDLE table section: %s", e.what());
                    }
                    
                    for (int m = 0; m < maxThrCount; ++m)
                    {
                        try
                        {
                            bcmThrCh.thrTb = thr_table_t{{2,m}};
                            std::cout << "    STD[" << m << "]    ";
                            
                            std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMinEn(bcmThrCh);
                            std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMin(bcmThrCh);
                            std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMaxEn(bcmThrCh);
                            std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMax(bcmThrCh);
                            std::cout << std::endl;

                        }
                        catch (std::exception &e)
                        {
                            printf("   Error on STD table section: %s", e.what());
                        }
                    }


                    for (int m = 0; m < maxThrCount; ++m)
                    {
                        try
                        {
                    
                            bcmThrCh.thrTb = thr_table_t{{1,0}};
                            std::cout << "    ALT[" << m << "]    ";
                                
                            std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMinEn(bcmThrCh);
                            std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMin(bcmThrCh);
                            std::cout << std::setw(8) << std::boolalpha << myMpsBcm[i]->getThresholdMaxEn(bcmThrCh);
                            std::cout << std::setw(8) << myMpsBcm[i]->getThresholdMax(bcmThrCh);
                            std::cout << std::endl;
                        }
                        catch (std::exception &e)
                        {
                            printf("   Error on ALT table section: %s", e.what());
                        }

                    }


                    std::cout << "    ==========================================" << std::endl;
                    std::cout << std::endl;
                    std::cout << "  ===============================================" << std::endl;
                    std::cout << std::endl;
                }
                catch (std::exception &e)
                {
                    printf("  Error channel info section: %s\n", e.what());
                }

            }

            std::cout << "====================================================" << std::endl;
            std::cout << std::endl;
        }

    }
    catch (CPSWError &e)
    {
        printf("CPSW error: %s not found!\n", e.getInfo().c_str());
        return 1;
    }

    return 0;
}