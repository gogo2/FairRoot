/* 
 * File:   SimpleTreeReader.h
 * Author: winckler
 *
 * Created on November 25, 2014, 11:17 AM
 */

#ifndef SIMPLEROOTSAMPLER_H
#define	SIMPLEROOTSAMPLER_H


// std
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <stdint.h>

// ROOT
#include "Rtypes.h"
#include "TFile.h"
#include "TTree.h"
#include "TClonesArray.h"

// FairRoot
#include "FairMQLogger.h"
#include "FairMQMessage.h"

template <typename DataType>
class base_SimpleTreeReader 
{
protected:
    typedef DataType* DataType_ptr;
    typedef DataType& DataType_ref;
public:
    base_SimpleTreeReader()
        : fDataBranch(nullptr)
        , fFileName()
        , fTreeName()
        , fBranchName()
        , fInputFile(nullptr)
        , fTree(nullptr)
        , fIndex(0)
        , fIndexMax(0)
    {}

    virtual ~base_SimpleTreeReader()
    {
        if (fInputFile)
        {
            fInputFile->Close();
            delete fInputFile;
        }
    }

    void SetFileProperties(const std::string &filename, const std::string &treename, const std::string &branchname)
    {
        fFileName = filename;
        fTreeName = treename;
        fBranchName = branchname;
    }

    // template < std::enable_if<std::is_base_of<TObject, DataType>::value,int> = 0>
    void InitSampler()
    {
        fInputFile = TFile::Open(fFileName.c_str(), "READ");
        if (fInputFile)
        {
            fTree = (TTree*)fInputFile->Get(fTreeName.c_str());
            if (fTree)
            {
                fTree->SetBranchAddress(fBranchName.c_str(),&fDataBranch);
                fIndexMax=fTree->GetEntries();
            }
            else
            {
                LOG(ERROR)<<"Could not find tree "<<fTreeName;
            }
        }
        else
        {
            LOG(ERROR)<<"Could not open file "<<fFileName<<" in SimpleTreeReader::InitSampler()";
        }
        
    }
    
    
    
    
    /// ///////////////////////////////////////////////////////////////////////////////////////    
    // simple example of the zmq multipart 
    // for each available socket of the output channel, it will group N consecutive messages (via zmq multipart) before sending them
    // the GetSocketNumber() and SendPart(int j) are callback functions of the host (derived) class GenericSampler<sampler,output>
    template<int N>
    void MultiPartTask()
    {
        for(int j(0); j<GetSocketNumber(); j++)
        {
            SendMultiPart<N>(j);
        }
        
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    //template<int N, typename = std::enable_if<N!=0> >
    template<int N>
    void SendMultiPart(int socketID)//, bool recursive=false)
    {
        //bool recursive=true;
        fIndex=GetCurrentIndex();
        if(fIndex+N<GetNumberOfEvent())
        {
            for(int i(0); i<N; i++)
                SendPart(socketID);// callback that does the zmq multipart AND increment the current index (Event number) in generic sampler
        }
        //else
        //{
        //    if(recursive)
        //        SendMultiPart<N-1>(socketID);// in case we want to group the remaining messages
        //}
    }
    
    // TODO : finish the recursive template grouping 
    //template<int N, typename = std::enable_if<N==0> >
    //void SendMultiPart(int socketID,int=0) {}
    
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    void SetIndex(int64_t Event)
    {
        fIndex = Event;
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    DataType_ptr GetOutData()
    {
        return GetOutData(fIndex);
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    DataType_ptr GetOutData(int64_t Event)
    {
        fTree->GetEntry(Event);
        return fDataBranch;
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    int64_t GetNumberOfEvent()
    {
        if (fTree) 
            return fIndexMax;
        else 
            return 0;
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    template<typename T>
    std::vector< std::vector<T> > GetDataVector()
    {
        std::vector<std::vector<T> > Allobj;
        std::vector<T> TempObj;
        if (std::is_same<DataType,TClonesArray>::value)
        {
            for (int64_t i(0);i<fTree->GetEntries() ;i++)
            {
                TempObj.clear();
                fTree->GetEntry(i);
                for (int64_t iobj = 0; iobj < fDataBranch->GetEntriesFast(); ++iobj)
                {
                    T* Data_i = reinterpret_cast<T*>(fDataBranch->At(iobj));
                    if (!Data_i)
                        continue;
                    TempObj.push_back(*Data_i);
                }
                Allobj.push_back(TempObj);
            }
        }
        else
        {
            for (int64_t i(0);i<fTree->GetEntries() ;i++)
            {
                TempObj.clear();
                fTree->GetEntry(i);
                T Data_i=*fDataBranch;
                TempObj.push_back(Data_i);
                Allobj.push_back(TempObj);
            }
        }
        return Allobj;
    }

    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    // provides a callback to the Sampler.
    void BindSendPart(std::function<void(int)> callback)
    {
        SendPart = callback;
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    void BindGetSocketNumber(std::function<int()> callback)
    {
        GetSocketNumber = callback;
    }
    
    /// ///////////////////////////////////////////////////////////////////////////////////////
    void BindGetCurrentIndex(std::function<int()> callback)
    {
        GetCurrentIndex = callback;
    }
    
private:
    /// ///////////////////////////////////////////////////////////////////////////////////////
    std::function<void(int)> SendPart;    // function pointer for the Sampler callback.
    std::function<int()> GetSocketNumber; // function pointer for the Sampler callback.
    std::function<int()> GetCurrentIndex; // function pointer for the Sampler callback.
    /// ///////////////////////////////////////////////////////////////////////////////////////
    DataType_ptr fDataBranch;// data type of the branch you want to extract
    std::string fFileName;
    std::string fTreeName;
    std::string fBranchName;
    TFile* fInputFile;
    TTree* fTree;
    int64_t fIndex;
    int64_t fIndexMax;
};

template<typename T>
using SimpleTreeReader = base_SimpleTreeReader<T>;

#endif	/* SIMPLEROOTSAMPLER_H */

