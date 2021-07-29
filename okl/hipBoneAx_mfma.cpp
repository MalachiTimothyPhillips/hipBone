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

#if p_N!=15
#error "Not today"
#endif

#define p_pad 1

@kernel void hipBoneAx_mfma(const dlong Nelements,
                            @restrict const  dlong  *  elementList,
                            @restrict const  dlong  *  GlobalToLocal,
                            @restrict const  dfloat *  ggeo,
                            @restrict const  dfloat *  D,
                            @restrict const  dfloat *  S,
                            @restrict const  dfloat *  MM,
                            const dfloat lambda,
                            @restrict const  dfloat *  q,
                                  @restrict dfloat *  Aq){

  const int e = blockIdx.x;

  // using dfloat4 = __attribute__((__vector_size__(4 * sizeof(dfloat)))) dfloat;

  __shared__ dfloat s_D[p_Nq][p_Nq+p_pad];
  __shared__ dfloat s_DT[p_Nq][p_Nq+p_pad];
  __shared__ dfloat s_q[p_Nq][p_Nq+p_pad];

  dfloat r_GDqt, r_Aqk;

  // register array to hold u(i,j,0:N) private to thread
  dfloat r_q[p_Nq];
  // array for results Au(i,j,0:N)
  dfloat r_Aq[p_Nq];

  dlong element;

  dfloat4 r_qr;

  const int i = threadIdx.x;
  const int j = threadIdx.y;

  //load D into local memory
  // s_D[i][j] = d \phi_i at node j
  const dfloat Dji = D[p_Nq*j+i];// D is column major
  s_D[j][i] = Dji;
  s_DT[i][j] = Dji;

  element = elementList[e];
  const dlong base = i + j*p_Nq + element*p_Np;

  // load pencil of u into register
  #pragma unroll p_Nq
  for (int k=0;k<p_Nq;k++) {
    const dlong id = GlobalToLocal[base + k*p_Nq*p_Nq];
    r_q[k] = (id!=-1) ? q[id] : 0.0;
  }

  #pragma unroll p_Nq
  for (int k=0;k<p_Nq;k++) {
    r_Aq[k] = 0.0;
  }
  __syncthreads();

  for(int k=0;k<p_Nq;++k){

    s_q[j][i] = r_q[k];
    __syncthreads();

    dfloat r_G00, r_G01, r_G02, r_G11, r_G12, r_G22, r_GwJ;

		if(r_e<Nelements){
			// prefetch geometric factors
			const dlong gbase = element*p_Nggeo*p_Np + k*p_Nq*p_Nq + j*p_Nq + i;

			r_GwJ = ggeo[gbase+p_GWJID*p_Np];
			r_G00 = ggeo[gbase+p_G00ID*p_Np];
			r_G01 = ggeo[gbase+p_G01ID*p_Np];
			r_G11 = ggeo[gbase+p_G11ID*p_Np];
			r_G12 = ggeo[gbase+p_G12ID*p_Np];
			r_G02 = ggeo[gbase+p_G02ID*p_Np];
			r_G22 = ggeo[gbase+p_G22ID*p_Np];
		}

    dfloat qr = 0.f;
    dfloat qs = 0.f;
    dfloat qt = 0;

    #pragma unroll p_Nq
    for (int m=0;m<p_Nq;m++) {
      qt += s_DT[m][k]*r_q[m];
    }

    #pragma unroll 4
    for (int m=0;m<4;m++) {
      dfloat A, B, C;
      B = s_DT[4*m+j%4][i];
      A = s_q[i%4+j/4][4*m+j%4];

      qr = __builtin_amdgcn_mfma_f64_4x4x4f64(A, B, qr, 0, 0, 0); // x-deriv
    }

    #pragma unroll 4
    for (int m=0;m<4;m++) {
      dfloat A, B, C;
      B = s_q[4*m+j%4][i];
      A = s_DT[4*m+j%4][i%4+j/4];

      // Ordering of the result is the same as the B matrix
      qs = __builtin_amdgcn_mfma_f64_4x4x4f64(A, B, qs, 0, 0, 0); // y-deriv
    }


    s_v[j][i] = (r_G00*qr + r_G01*qs + r_G02*qt);
    s_w[j][i] = (r_G01*qr + r_G11*qs + r_G12*qt);
    r_GDqt    = (r_G02*qr + r_G12*qs + r_G22*qt);

    r_Aqk = r_GwJ*lambda*r_q[k];
    __synthreads();

    #pragma unroll p_Nq
    for (int m=0;m<p_Nq;m++) {
      r_Aq[m] += s_D[k][m]*r_GDqt;
    }

    #pragma unroll 4
    for (int m=0;m<4;m++) {
      dfloat A, B;
      B = s_w[4*m+j%4][i];
      A = s_D[4*m+j%4][i%4+j/4];

      // Ordering of the result is the same as the B matrix
      r_Aqk = __builtin_amdgcn_mfma_f64_4x4x4f64(A, B, r_Aqk, 0, 0, 0); // y-deriv
		}

    #pragma unroll 4
		for (int m=0;m<4;m++) {
      dfloat A, B, C;
      B = s_D[4*m+j%4][i];
      A = s_v[i%4+j/4][4*m+j%4];

      r_Aqk = __builtin_amdgcn_mfma_f64_4x4x4f64(A, B, r_Aqk, 0, 0, 0); // x-deriv
    }

    r_Aq[k] += r_Aqk;
    __syncthreads();
  } //end Layer by layer

  // write out
  if(e<Nelements){
    const dlong id = element*p_Np + j*p_Nq + i;

    #pragma unroll p_Nq
    for (int k=0;k<p_Nq;k++) {
      Aq[id+k*p_Nq*p_Nq] = r_Aq[k];
    }
  }
}
