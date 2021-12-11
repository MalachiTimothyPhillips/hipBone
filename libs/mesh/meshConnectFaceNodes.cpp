/*

The MIT License (MIT)

Copyright (c) 2020 Tim Warburton, Noel Chalmers, Jesse Chan, Ali Karakus

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#include "mesh.hpp"

namespace libp {

// serial face-node to face-node connection
void mesh_t::ConnectFaceNodes(){

  const dfloat NODETOL = 1.0e-5;

  dfloat V0[4][2] = {{-1.0,-1.0},{ 1.0,-1.0},{ 1.0, 1.0},{-1.0, 1.0}};
  dfloat V1[4][2] = {{-1.0,-1.0},{-1.0, 1.0},{ 1.0, 1.0},{ 1.0,-1.0}};

  dfloat EX0[Nverts], EY0[Nverts];
  dfloat EX1[Nverts], EY1[Nverts];

  dfloat *x0 = (dfloat*) malloc(Nfp*sizeof(dfloat));
  dfloat *y0 = (dfloat*) malloc(Nfp*sizeof(dfloat));

  dfloat *x1 = (dfloat*) malloc(Nfp*sizeof(dfloat));
  dfloat *y1 = (dfloat*) malloc(Nfp*sizeof(dfloat));

  /* Build the permutation array R */
  int *R = (int*) malloc(Nfaces*Nfaces*Nverts*Nfp*sizeof(int));

  for (int fM=0;fM<Nfaces;fM++) {

    for (int v=0;v<Nverts;v++) {
      EX0[v] = 0.0; EY0[v] = 0.0;
    }
    //setup top element with face fM on the bottom
    for (int v=0;v<NfaceVertices;v++) {
      int fv = faceVertices[fM*NfaceVertices + v];
      EX0[fv] = V0[v][0]; EY0[fv] = V0[v][1];
    }

    for(int n=0;n<Nfp;++n){ /* for each face node */
      const int fn = faceNodes[fM*Nfp+n];

      /* (r,s,t) coordinates of interpolation nodes*/
      dfloat rn = r[fn];
      dfloat sn = s[fn];
      dfloat tn = t[fn];

      /* physical coordinate of interpolation node */
      x0[n] =
        +0.125*(1-rn)*(1-sn)*(1-tn)*EX0[0]
        +0.125*(1+rn)*(1-sn)*(1-tn)*EX0[1]
        +0.125*(1+rn)*(1+sn)*(1-tn)*EX0[2]
        +0.125*(1-rn)*(1+sn)*(1-tn)*EX0[3]
        +0.125*(1-rn)*(1-sn)*(1+tn)*EX0[4]
        +0.125*(1+rn)*(1-sn)*(1+tn)*EX0[5]
        +0.125*(1+rn)*(1+sn)*(1+tn)*EX0[6]
        +0.125*(1-rn)*(1+sn)*(1+tn)*EX0[7];

      y0[n] =
        +0.125*(1-rn)*(1-sn)*(1-tn)*EY0[0]
        +0.125*(1+rn)*(1-sn)*(1-tn)*EY0[1]
        +0.125*(1+rn)*(1+sn)*(1-tn)*EY0[2]
        +0.125*(1-rn)*(1+sn)*(1-tn)*EY0[3]
        +0.125*(1-rn)*(1-sn)*(1+tn)*EY0[4]
        +0.125*(1+rn)*(1-sn)*(1+tn)*EY0[5]
        +0.125*(1+rn)*(1+sn)*(1+tn)*EY0[6]
        +0.125*(1-rn)*(1+sn)*(1+tn)*EY0[7];
    }

    for (int fP=0;fP<Nfaces;fP++) { /*For each neighbor face */
      for (int rot=0;rot<Nfaces;rot++) { /* For each face rotation */
        // Zero vertices
        for (int v=0;v<Nverts;v++) {
          EX1[v] = 0.0; EY1[v] = 0.0;
        }
        //setup bottom element with face fP on the top
        for (int v=0;v<NfaceVertices;v++) {
          int fv = faceVertices[fP*NfaceVertices + ((v+rot)%NfaceVertices)];
          EX1[fv] = V1[v][0]; EY1[fv] = V1[v][1];
        }

        for(int n=0;n<Nfp;++n){ /* for each node */
          const int fn = faceNodes[fP*Nfp+n];

          /* (r,s,t) coordinates of interpolation nodes*/
          dfloat rn = r[fn];
          dfloat sn = s[fn];
          dfloat tn = t[fn];

          /* physical coordinate of interpolation node */
          x1[n] =  0.125*(1-rn)*(1-sn)*(1-tn)*EX1[0]
                  +0.125*(1+rn)*(1-sn)*(1-tn)*EX1[1]
                  +0.125*(1+rn)*(1+sn)*(1-tn)*EX1[2]
                  +0.125*(1-rn)*(1+sn)*(1-tn)*EX1[3]
                  +0.125*(1-rn)*(1-sn)*(1+tn)*EX1[4]
                  +0.125*(1+rn)*(1-sn)*(1+tn)*EX1[5]
                  +0.125*(1+rn)*(1+sn)*(1+tn)*EX1[6]
                  +0.125*(1-rn)*(1+sn)*(1+tn)*EX1[7];

          y1[n] =  0.125*(1-rn)*(1-sn)*(1-tn)*EY1[0]
                  +0.125*(1+rn)*(1-sn)*(1-tn)*EY1[1]
                  +0.125*(1+rn)*(1+sn)*(1-tn)*EY1[2]
                  +0.125*(1-rn)*(1+sn)*(1-tn)*EY1[3]
                  +0.125*(1-rn)*(1-sn)*(1+tn)*EY1[4]
                  +0.125*(1+rn)*(1-sn)*(1+tn)*EY1[5]
                  +0.125*(1+rn)*(1+sn)*(1+tn)*EY1[6]
                  +0.125*(1-rn)*(1+sn)*(1+tn)*EY1[7];
        }

        /* for each node on this face find the neighbor node */
        for(int n=0;n<Nfp;++n){
          const dfloat xM = x0[n];
          const dfloat yM = y0[n];

          int m=0;
          for(;m<Nfp;++m){ /* for each neighbor node */
            const dfloat xP = x1[m];
            const dfloat yP = y1[m];

            /* distance between target and neighbor node */
            const dfloat dist = pow(xM-xP,2) + pow(yM-yP,2);

            /* if neighbor node is close to target, match */
            if(dist<NODETOL){
              R[fM*Nfaces*NfaceVertices*Nfp
                + fP*NfaceVertices*Nfp
                + rot*Nfp + n] = m;
              break;
            }
          }

          /*Check*/
          const dfloat xP = x1[m];
          const dfloat yP = y1[m];

          /* distance between target and neighbor node */
          const dfloat dist = pow(xM-xP,2) + pow(yM-yP,2);
          if(dist>NODETOL){
            //This shouldn't happen
            std::stringstream ss;
            ss << "Unable to match face node, face: " << fM
               << ", matching face: " << fP
               << ", rotation: " << rot
               << ", node: " << n
               << ". Is the reference node set not symmetric?";
            HIPBONE_ABORT(ss.str())
          }
        }
      }
    }
  }

  free(x0); free(y0);
  free(x1); free(y1);

  /* volume indices of the interior and exterior face nodes for each element */
  vmapM = (dlong*) calloc(Nfp*Nfaces*Nelements, sizeof(dlong));
  vmapP = (dlong*) calloc(Nfp*Nfaces*Nelements, sizeof(dlong));

  /* assume elements already connected */
  for(dlong eM=0;eM<Nelements;++eM){
    for(int fM=0;fM<Nfaces;++fM){
      dlong eP = EToE[eM*Nfaces+fM];
      int fP = EToF[eM*Nfaces+fM];
      if(eP<0 || fP<0){ // fake connections for unconnected faces
        for(int nM=0;nM<Nfp;++nM){
          const int idM = faceNodes[fM*Nfp+nM];
          const dlong id = eM*Nfaces*Nfp + fM*Nfp + nM;
          vmapM[id] = idM + eM*Np;
          vmapP[id] = idM + eM*Np;
        }
      } else {
        //Find the rotation of the face from where the first vertex of the face is
        hlong vf0P = EToV[eP*Nverts + faceVertices[fP*NfaceVertices+0]];
        int rot=0;
        for (;rot<NfaceVertices;++rot) {
          hlong vfM = EToV[eM*Nverts + faceVertices[fM*NfaceVertices+rot]];
          if (vfM == vf0P) break;
        }

        /* for each node on this face use the permuation array
           to select the neighbor node */
        for(int nM=0;nM<Nfp;++nM){
          const int nP  = R[fM*Nfaces*Nfp*NfaceVertices
                            + fP*Nfp*NfaceVertices
                            + rot*Nfp + nM];
          const int idM = faceNodes[fM*Nfp+nM];
          const int idP = faceNodes[fP*Nfp+nP];

          const dlong id = eM*Nfaces*Nfp + fM*Nfp + nM;
          vmapM[id] = idM + eM*Np;
          vmapP[id] = idP + eP*Np;

      #if 0
        /*Sanity check*/
        dfloat xnM = x[idM + eM*Np];
        dfloat ynM = y[idM + eM*Np];
        dfloat znM = z[idM + eM*Np];
        dfloat xnP = x[idP + eP*Np];
        dfloat ynP = y[idP + eP*Np];
        dfloat znP = z[idP + eP*Np];
        const dfloat dist = pow(xnM-xnP,2) + pow(ynM-ynP,2) + pow(znM-znP,2);
        if (dist>NODETOL)
          printf("Mismatch?: Element %d, face %d, node %d, xM = (%f, %f, %f), xP = (%f, %f, %f)\n",
                  eM, fM, nM, xnM, ynM, znM, xnP, ynP, znP);
      #endif
        }
      }
    }
  }

//      printf("connecting (%d,%d) to (%d,%d) [ vmapM %d to vmapP %d ]\n",
//             e,f,eP,fP, vmapM[id], vmapP[id]);
  free(R);
}

} //namespace libp