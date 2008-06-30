#pragma ident "$Id$"

/**
 * @file SatArcMarker.cpp
 * This class keeps track of satellite arcs caused by cycle slips.
 */

//============================================================================
//
//  This file is part of GPSTk, the GPS Toolkit.
//
//  The GPSTk is free software; you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published
//  by the Free Software Foundation; either version 2.1 of the License, or
//  any later version.
//
//  The GPSTk is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with GPSTk; if not, write to the Free Software Foundation,
//  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//  Dagoberto Salazar - gAGE ( http://www.gage.es ). 2008
//
//============================================================================


#include "SatArcMarker.hpp"


namespace gpstk
{

      // Index initially assigned to this class
   int SatArcMarker::classIndex = 2900000;


      // Returns an index identifying this object.
   int SatArcMarker::getIndex() const
   { return index; }


      // Returns a string identifying this object.
   std::string SatArcMarker::getClassName() const
   { return "SatArcMarker"; }


      /* Common constructor
       *
       * @param watchFlag        Cycle slip flag to be watched.
       * @param delUnstableSats  Whether unstable satellites will be deleted.
       * @param unstableTime     Number of seconds since last arc change
       *                         that a satellite will be considered as
       *                         unstable.
       */
   SatArcMarker::SatArcMarker( const TypeID& watchFlag,
                               const bool delUnstableSats,
                               const double unstableTime )
      : watchCSFlag(watchFlag), deleteUnstableSats(delUnstableSats)
   {
         // Check unstableTime value
      if (unstableTime > 0.0)
      {
         unstablePeriod = unstableTime;
      }
      else
      {
         unstablePeriod = 0.0;
      }

      setIndex();
   }


      /* Method to set the number of seconds since last arc change that a
       *  satellite will be considered as unstable.
       *
       * @param unstableTime     Number of seconds since last arc change
       *                         that a satellite will be considered as
       *                         unstable.
       */
   SatArcMarker& SatArcMarker::setUnstablePeriod(const double unstableTime)
   {
         // Check unstableTime value
      if (unstableTime > 0.0)
      {
         unstablePeriod = unstableTime;
      }
      else
      {
         unstablePeriod = 0.0;
      }

      return (*this);
   }


      /* Returns a satTypeValueMap object, adding the new data generated
       *  when calling this object.
       *
       * @param epoch     Time of observations.
       * @param gData     Data object holding the data.
       * @param epochflag Epoch flag.
       */
   satTypeValueMap& SatArcMarker::Process( const DayTime& epoch,
                                           satTypeValueMap& gData,
                                           const short& epochflag )
   {
      double flag(0.0);

      SatIDSet satRejectedSet;

         // Loop through all the satellites
      satTypeValueMap::iterator it;
      for (it = gData.begin(); it != gData.end(); ++it) 
      {
         try
         {
               // Try to extract the CS flag value
            flag = (*it).second(watchCSFlag);
         }
         catch(...)
         {
               // If flag is missing, then schedule this satellite
               // for removal
            satRejectedSet.insert( (*it).first );
            continue;
         }

            // If there was a CS, then increment the value of "TypeID::satArc"
         if ( flag > 0.0 )
         {
               // Check if satellite currently has entries
            std::map<SatID, double>::const_iterator itArc;
            itArc = satArcMap.find( (*it).first );
            if( itArc == satArcMap.end() ) 
            {
                  // If it doesn't have an entry, insert one
               satArcMap[ (*it).first ] = 0.0;
               satArcChangeMap[ (*it).first ] = DayTime::BEGINNING_OF_TIME;
            };

            std::map<SatID, double>::const_iterator itFlag;
            itFlag = prevCSFlagMap.find( (*it).first );
            if( itFlag == prevCSFlagMap.end() ) 
            {
                  // If it doesn't have an entry, insert one
               prevCSFlagMap[ (*it).first ] = 0.0;
            };

               // Increase the satellite arc number if it wasn't done before
               // and if "unstable period" is over
            if( ( prevCSFlagMap[ (*it).first ] < 1.0 ) && 
                (std::abs(epoch-satArcChangeMap[(*it).first])>unstablePeriod))
            {
               satArcMap[ (*it).first ] = satArcMap[ (*it).first ] + 1.0;

                  // Update arc change epoch only if this is NOT the first arc
               if( satArcMap[ (*it).first ] > 1.0 )
               {
                  satArcChangeMap[ (*it).first ] = epoch;
               }
            }

               // Check if satellite is unstable and if we want to remove it
            if ( deleteUnstableSats &&
                 (std::abs(epoch-satArcChangeMap[(*it).first])<=unstablePeriod))
            {
               satRejectedSet.insert( (*it).first );
            }

         }

            // We will insert the satellite arc number
         (*it).second[TypeID::satArc] = satArcMap[ (*it).first ];

            // Store the CS flag value
         prevCSFlagMap[ (*it).first ] = (*it).second(watchCSFlag);

      }

         // Remove satellites with missing data
      gData.removeSatID(satRejectedSet);

      return gData;

   }


      /* Returns a gnnsRinex object, adding the new data generated when
       *  calling this object.
       *
       * @param gData    Data object holding the data.
       */
   gnssRinex& SatArcMarker::Process(gnssRinex& gData)
   {
      Process(gData.header.epoch, gData.body, gData.header.epochFlag);
      return gData;
   }


} // end namespace gpstk