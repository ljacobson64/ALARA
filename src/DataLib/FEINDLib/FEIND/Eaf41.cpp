#include <iostream>
#include <cassert>
#include <cctype>
#include <cstdlib>

#include "Eaf41.h"
#include "LibDefine.h"
#include "RamLib.h"


using namespace FEIND;
using namespace std;

const int Eaf41::NumGroups = 175;

Eaf41::Eaf41(const LibDefine& lib) :
  FileName(lib.Args[0].c_str()),
  InFile(lib.Args[0].c_str())
{
  Cpt.push_back(PROTON);
  Cpt.push_back(DEUTERON);
  Cpt.push_back(TRITON);
  Cpt.push_back(HELIUM3);
  Cpt.push_back(ALPHA);
}

void Eaf41::LoadLibrary() throw(ExFileOpen, ExEmptyXSec)
{
  int non_zero,i;
  XSecType fission_type;
  bool is_fission;
  string str;

  Kza parent_kza;
  Kza daughter_kza;

  string path_str;
  string daughter_str;
  
  Path path;

  if(!InFile.is_open())
    throw ExFileOpen("Eaf41::LoadLibrary() function", FileName);

  SkipHeader();

  getline(InFile, str, '\n');
  while(!InFile.eof())
    {
      is_fission = false;

      // Get the parent:
      parent_kza = atoi(str.substr(0,7).c_str());

      // Now get the number of non-zero groups:
      non_zero = atoi(str.substr(14,3).c_str());

      // Get path and daughter info:
      path_str = str.substr(28,5);
      daughter_str = str.substr(34,7);

      if(path_str[2] == 'F')
        {
          // Fission Reaction
          // Find the type of fission:
          fission_type = FissionType(path_str[0]);
          is_fission = true;
        }
      else
        {
          path = ConvertPath(path_str);          
          daughter_kza = DaughterEtoF(daughter_str);
        }

      // Skip 2 lines:
      getline(InFile,str,'\n');
      getline(InFile,str,'\n');

      path.CrossSection.Reset(NumGroups);

      for(i = 0; i < 175; i++)
        {
          if(i < non_zero)
            {
              InFile >> str;
              path.CrossSection[i] = atof(str.c_str());
            }
          else
            {
              path.CrossSection[i] = 0.0;
            }
        }

      if(!is_fission)
        {
          // Add the parent, parent/daughter contributions:
          Library.SetDCs(parent_kza, daughter_kza, TOTAL_CS, 
                         path.CrossSection, true);

          // Add the parent/daughter/path data:
          Library.AddPath(parent_kza, daughter_kza, path);

          // Add secondary daughters to the daughter list:
          AddCPPCS(parent_kza,path);
        }
      else
        {
          // Set the fission cross-section:
          Library.SetPCs(parent_kza, fission_type, path.CrossSection);
        }
      
      // Every reaction in this file format contributes to the total
      // cross-section
      Library.SetPCs(parent_kza, TOTAL_CS, path.CrossSection, true);      
      
      // Get the header for this reaction:
      getline(InFile, str, '\n');
      if(str == "") getline(InFile, str, '\n');
    }
}

void Eaf41::SkipHeader()
{
  string str;

  while(str[0] != '#')
    getline(InFile, str, '\n');
}

Path Eaf41::ConvertPath(const string& str)
{
  Path ret;
  unsigned int i;
  int num_part = 1;

  string character;

  // First character in str is the projectile:
  ret.Projectile = ParticleEtoF(str[0]);

  for(i = 2; i < str.size(); i++)
    {
      character = str[i];
      // Ignore blank spaces:
      if(str[i] != ' ')
        {
          // Check for a number:
          if( !atoi(character.c_str()) )
            {
              // Not a number:
              ret.Emitted[ParticleEtoF(str[i])] += num_part;
              num_part = 1;
              
            }
          else
            {
              // It is a number:
              num_part = atoi(character.c_str());
            }
        }
    }

  return ret;
}

int Eaf41::ParticleEtoF(const char part)
{
  switch(toupper(part))
    {
    case 'N':
      return NEUTRON;
    case 'G':
      return GAMMA;
    case 'D':
      return DEUTERON;
    case 'A':
      return ALPHA;
    case 'T':
      return TRITON;
    case 'P':
      return PROTON;
    case 'H':
      return HELIUM3;
    }

  return 0;
}

Kza Eaf41::DaughterEtoF(const string& daughter)
{
  string element;
  int z;
  int a;
  int iso = 0;

  // Take care of Z:
  if(daughter[1] == ' ')
    {
      // element symbol is single character...
      // ie C, O, N...
      element = daughter[0];
    }
  else
    {
      // element symbol is two characters...
      // ie He, Be, Xe...
      element = daughter.substr(0,2);
      element[0] = toupper(element[0]);
      element[1] = tolower(element[1]);
    }
  z = GetAtomicNumber(element);

  // Take care of A:
  a = atoi(daughter.substr(2,3).c_str());

  // Take care of Isomeric state:
  if(daughter.find('M') != string::npos)
    iso = atoi(daughter.substr(6,1).c_str()); 

  return z*10000 + a*10 + iso;
}

XSecType Eaf41::FissionType(const char projectile)
{
  switch(ParticleEtoF(projectile))
    {
    case NEUTRON:
      return NEUTRON_FISSION_CS;
    }
  assert(false);
  exit(EXIT_FAILURE);
}

void Eaf41::AddCPPCS(Kza parent, Path& path) throw(ExEmptyXSec)
{
  // This function will add any charged particles produced to the production
  // cross-section.

  XSec cs;
  unsigned int i;

  for(i = 0; i < Cpt.size(); i++)
    {
      // EXCEPTION - Cpt find

      if(path.Emitted.find(Cpt[i]) != path.Emitted.end())
        {
          assert(path.Emitted[Cpt[i]]);

          cs = path.CrossSection;

          if(path.Emitted[Cpt[i]] > 1)
            {
              cs *= path.Emitted[Cpt[i]];
            }

          Library.SetDCs(parent, Cpt[i], TOTAL_CS, cs, true);
        }
    }
}
