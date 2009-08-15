/* RoadmapNativeFile.h - Symbian sound implementation for Roadmap
 *
 * LICENSE:
 *
 *   Copyright 2008 Giant Steps Ltd.
 *   Copyright 2008 Ehud Shabtai
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License V2 as published by
 *   the Free Software Foundation.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See roadmap_file.h
 */

#ifndef _ROADMAP_NATIVE_FILE__H_
#define _ROADMAP_NATIVE_FILE__H_

extern "C" {
#include "roadmap_file.h"
}

#include <f32file.h>

class CRoadMapNativeFile : public CBase
{
public:
  enum EFileOp
  {
    EFileOpRead,
    EFileOpWrite,
    EFileOpAppend,
    EFileOpReadWrite
  };

  //  ctor/dtor
  CRoadMapNativeFile();
  virtual ~CRoadMapNativeFile();
  void CRoadMapNativeFile::ConstructL(TFileName &fileName,
                                      CRoadMapNativeFile::EFileOp fileOp,
                                      TInt &aErrorCode);

  static CRoadMapNativeFile* NewL(TFileName &fileName,
                                  CRoadMapNativeFile::EFileOp fileOp,
                                  TInt &aErrorCode);
  static CRoadMapNativeFile* NewLC( TFileName &fileName,
                                    CRoadMapNativeFile::EFileOp fileOp,
                                    TInt &aErrorCode);

  int Read(void *data, int length);
  int Write(const void *data, int length);
  int GetSize(int& fileSize);
  int SetSize(int fileSize);
  int Seek(TSeek method, int pos);

private:
  RFs iSession;
  RFile iFile;
  bool m_IsFileValid;
};

#endif  //  #ifndef _ROADMAP_NATIVE_FILE__H_
