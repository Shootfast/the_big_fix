#include <stdint.h>
#include <libdragon.h>
#include <texture_manager.h>

/**
 * Decodes a single pixel based on the custom 4x4 block format.
 * Base logic from the decodeBlockNew macro.
 */
static inline uint16_t decode_pixel(uint32_t pixel_addr) {
    const uint32_t ADDR_MASK = ~0xF;
    
    uint32_t grid_index = pixel_addr & 0xF;

    uint32_t base_addr = pixel_addr & ADDR_MASK;
    
    uint32_t indices = *(uint32_t*)(base_addr + 8);

	indices >>= (grid_index << 1);
	indices &= 0x6;

	// convert RGBA32 to RGB555
	//if (base_addr == 0x800000000){
	//	uint8_t r = (pixel_addr >> 16) & 0xFF;
	//	uint8_t g = (pixel_addr >> 8) & 0xFF;
	//	uint8_t b = (pixel_addr )& 0xFF;
	//	return ((r >> 3) << 10) |
	//		   ((g >> 3) << 5)  |
	//		    (b >> 3);
	uint16_t result = *(uint16_t*)(base_addr + indices);
	return result;
}


uint32_t apply_texture(uint32_t* fbTexIn, uint32_t* fbTexInEnd, uint16_t* fbOut64) {

    uint32_t p0, p1, p2, p3;
    uint16_t b0, b1, b2, b3;
#if(1)
    while (fbTexIn < fbTexInEnd) {
        // Each loop iteration processes 8 pixels (32 bytes of input)
        // MIPS used: lwr p0Addr, 2(fbTexIn) -> then 6, 10, 14...
        // This extracts the upper 24 bits (Index, U, V) and keeps 0x80 in the MSB
        
        // --- First 4 pixels ---
        p0 = 0x80000000 | (fbTexIn[0] >> 8);
        p1 = 0x80000000 | (fbTexIn[1] >> 8);
        p2 = 0x80000000 | (fbTexIn[2] >> 8);
        p3 = 0x80000000 | (fbTexIn[3] >> 8);

        b0 = decode_pixel(p0);
        b1 = decode_pixel(p1);
        fbOut64[0] = b0; 
        
        b2 = decode_pixel(p2);
        fbOut64[1] = b1; 
        
        b3 = decode_pixel(p3);
        fbOut64[2] = b2; 
        fbOut64[3] = b3; 

        // --- Next 4 pixels ---
        p0 = 0x80000000 | (fbTexIn[4] >> 8);
        p1 = 0x80000000 | (fbTexIn[5] >> 8);
        p2 = 0x80000000 | (fbTexIn[6] >> 8);
        p3 = 0x80000000 | (fbTexIn[7] >> 8);

        b0 = decode_pixel(p0);
        b1 = decode_pixel(p1);
        fbOut64[4] = b0; 
        
        b2 = decode_pixel(p2);
        fbOut64[5] = b1; 
        
        b3 = decode_pixel(p3);
        fbOut64[6] = b2; 
        fbOut64[7] = b3; 

        // Increment pointers
        fbTexIn += 8;   // We read 8 uint32_t elements
        fbOut64 += 8;   // We wrote 8 uint16_t elements
	}
#else
	while (fbTexIn < fbTexInEnd) {
		// 1. Extract the first 24 bits (Index, U, V) 
		// 2. Set the 0x80 MSB to match the MIPS KSEG0 pointer setup
		uint32_t pAddr = 0x80000000 | (*fbTexIn >> 8);

		// 3. Decode the block using the macro-equivalent logic
		// 4. Store the resulting 16-bit halfword to the output
		*fbOut64 = decode_pixel(pAddr);

		// 5. Increment both pointers by one element
		fbTexIn++;
		fbOut64++;
	}
#endif
	return 0;
}
