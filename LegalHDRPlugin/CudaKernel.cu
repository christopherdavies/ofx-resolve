__device__ float Pq2Luma(float N) {
  float pq_m1 = 0.1593017578125; // ( 2610.0 / 4096.0 ) / 4.0;
  float pq_m2 = 78.84375; // ( 2523.0 / 4096.0 ) * 128.0;
  float pq_c1 = 0.8359375; // 3424.0 / 4096.0 or pq_c3 - pq_c2 + 1.0;
  float pq_c2 = 18.8515625; // ( 2413.0 / 4096.0 ) * 32.0;
  float pq_c3 = 18.6875; // ( 2392.0 / 4096.0 ) * 32.0;
  float pq_C = 10000.0;

  float Np = powf( N, 1.0 / pq_m2 );
  float L = Np - pq_c1;
  if ( L < 0.0 ) {
    L = 0.0;
  }
  L = L / ( pq_c2 - pq_c3 * Np );
  L = powf( L, 1.0 / pq_m1 );
  L = L * pq_C;
  return L; // returns cd/m^2
}

__device__ float Luma2Pq(float C) {
  float pq_m1 = 0.1593017578125; // ( 2610.0 / 4096.0 ) / 4.0;
  float pq_m2 = 78.84375; // ( 2523.0 / 4096.0 ) * 128.0;
  float pq_c1 = 0.8359375; // 3424.0 / 4096.0 or pq_c3 - pq_c2 + 1.0;
  float pq_c2 = 18.8515625; // ( 2413.0 / 4096.0 ) * 32.0;
  float pq_c3 = 18.6875; // ( 2392.0 / 4096.0 ) * 32.0;
  float pq_C = 10000.0;

  float L = C / pq_C;
  float Lm = powf( L, pq_m1 );
  float N = ( pq_c1 + pq_c2 * Lm ) / ( 1.0 + pq_c3 * Lm );
  N = powf( N, pq_m2 );
  return N;
}

__device__ float Luma(float R, float G, float B) {
  float lumaRec2020 = R * 0.2627f + G * 0.6780f + B * 0.0593f;
  return lumaRec2020;
} 


__global__ void LegalOverlayKernel(int p_Width, int p_Height, float p_Luminance, float p_OverlayR, float p_OverlayG, float p_OverlayB, int p_OverlayDisplay, int p_OverlayLuma, const float* p_Input, float* p_Output)
{
   const int x = blockIdx.x * blockDim.x + threadIdx.x;
   const int y = blockIdx.y * blockDim.y + threadIdx.y;

   if ((x < p_Width) && (y < p_Height))
   {
       const int index = ((y * p_Width) + x) * 4;
      
      float t = Luma2Pq(p_Luminance);
      int overlay = p_OverlayDisplay;
      int lumaWarn = p_OverlayLuma;

      float rOver, gOver, bOver;
      rOver = p_OverlayR / 100;
      gOver = p_OverlayG / 100;
      bOver = p_OverlayB / 100;

      float r, g, b, a;
      r = p_Input[index + 0];
      g = p_Input[index + 1];
      b = p_Input[index + 2];
      a = p_Input[index + 3];

      if  (lumaWarn && overlay) {
        float rY, gY, bY;
        rY = Pq2Luma(r);
        gY = Pq2Luma(g);
        bY = Pq2Luma(b);
        float luma = Luma(rY, gY, bY);
        float pqLuma = Luma2Pq(luma);
        if ( pqLuma >= t ) {
          // if any channel is over threshold, replace with overlay color
          r = rOver;
          g = gOver;
          b = bOver;
        }
      } else if (overlay && ( r >= t || g >= t || b >= t ) ) {
        r = rOver;
        g = gOver;
        b = bOver;
      }

      p_Output[index + 0] = r;
      p_Output[index + 1] = g;
      p_Output[index + 2] = b;
      p_Output[index + 3] = a;   
   }
}

void RunCudaKernel(int p_Width, int p_Height, float p_Luminance, float* p_OverlayRgb, int p_OverlayDisplay, int p_OverlayLuma, const float* p_Input, float* p_Output)
{
    dim3 threads(128, 1, 1);
    dim3 blocks(((p_Width + threads.x - 1) / threads.x), p_Height, 1);

    LegalOverlayKernel<<<blocks, threads>>>(p_Width, p_Height, p_Luminance, p_OverlayRgb[0], p_OverlayRgb[1], p_OverlayRgb[2], p_OverlayDisplay, p_OverlayLuma, p_Input, p_Output);
}


