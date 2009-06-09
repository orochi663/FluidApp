//
//  FluidZip_Delegate.m
//  FluidApp
//

#import "FluidZip_Delegate.h"


@implementation FluidZip_Delegate

- (void)awakeFromNib
{
	r_windows = [[NSMutableArray alloc] initWithCapacity:10];
}

- (void)dealloc
{
	[r_windows release];
	[super dealloc];
}

- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename
{
	NSString *extension = [filename pathExtension];
	if ([extension isEqualToString:@"jpg"]
		|| [extension isEqualToString:@"jpeg"]
		|| [extension isEqualToString:@"png"]
		|| [extension isEqualToString:@"vec"]
		|| [extension isEqualToString:@"tga"])
	{
		NSImage *i = nil;
		if ([[filename pathExtension] compare:@"vec"] == NSOrderedSame)
		{
			//Put in decompression code here...
			NSSize s = {640,480};
			i = [[NSImage alloc] initWithSize:s];
		}
		else
		{
			i = [[NSImage alloc] initWithContentsOfFile:filename];
		}
		
		[NSBundle loadNibNamed:@"FluidZip_Window" owner:self];
		[r_windows addObject:i_window];
		[i_window release];
		
		[i_window setImage:i];
		[i_window setTitle:[filename lastPathComponent]];
		[i_window setDelegate:self];
		
		[i release];
		
		return YES;
	}
	
	return NO;
}

- (IBAction)onMenuFileOpen:(id)in_src
{
	NSOpenPanel *nsp = [NSOpenPanel openPanel];
	
	[nsp setCanChooseFiles:YES];
	[nsp setCanChooseDirectories:NO];
	[nsp setResolvesAliases:YES];
	[nsp setAllowsMultipleSelection:NO];
	[nsp setTitle:@"Open Image"];
	
	if ([nsp runModalForTypes:[NSArray arrayWithObjects:
										@"jpg",@"jpeg",@"png",@"vec",@"tga",
										nil]] == NSOKButton)
	{
		[[NSDocumentController sharedDocumentController]
		 noteNewRecentDocumentURL:[nsp URL]];
		
		[self application:NSApp openFile:[nsp filename]];
	}
}


unsigned char clamp(float f)
{
	if (f < 0)
		return 0;
	if (f > 255)
		return 255;
	
	return (unsigned char)f;
}

- (IBAction)onMenuFormatImageCompress:(id)in_src
{
	if (m_focus == nil)		return;
	
	NSBitmapImageRep *bir = [NSBitmapImageRep alloc];
	[[m_focus image] lockFocus];
	[bir initWithFocusedViewRect:NSMakeRect(0, 0, [[m_focus image] size].width,
											[[m_focus image] size].height)];
	[[m_focus image] unlockFocus];
	
	
	NSBitmapImageRep *cpy = [[NSBitmapImageRep alloc]
		initWithBitmapDataPlanes:NULL pixelsWide:[[m_focus image] size].width
									pixelsHigh:[[m_focus image] size].height
						bitsPerSample:[bir bitsPerPixel]/[bir samplesPerPixel]
									samplesPerPixel:[bir samplesPerPixel]
									hasAlpha:[bir hasAlpha] isPlanar:NO
									colorSpaceName:NSCalibratedRGBColorSpace
									bytesPerRow:[bir bytesPerRow]
							 bitsPerPixel:[bir bitsPerPixel]];
	
	NSImage *i2 = [[NSImage alloc] initWithSize:[[m_focus image] size]];
	[i2 addRepresentation:cpy];
	
	//memcpy([cpy bitmapData], [bir bitmapData],
	//	   [bir bytesPerRow]*[i2 size].height);
	
	unsigned char *s = [bir bitmapData];
	unsigned char *d = [cpy bitmapData];
	
	//Now - apply a basic compression kernel (just to see results)
	int x,y,z;
	int w = [i2 size].width, h = [i2 size].height, bpp = [bir bitsPerPixel]/8;
	
	printf("Uncompressed image size: %ikb (32bit: %ikb)\n", w*h*bpp/1024, w*h*4*bpp/1024);
	int *indices = malloc(sizeof(int)*w*h);
	float *data = malloc(sizeof(float)*w*h*bpp);
	
	float *deriv = malloc(sizeof(float)*w*h);
	float *curv = malloc(sizeof(float)*w*h);
	
	
	//First, clean up the input?
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				if
//			}
//		}
//	}
	
	
	//First, compute the curvature...
	
	//Loop through the data accumulating curvature.  Once curvature change
	//exceeds a given threshold (on any component) create an entry.
	int memUsageReal = 0;
	int mem8bit = 0;
	for (y=0; y<h; y++)
	{
		//Clean up a bit
//		for (x=1; x<w-1; x++)
//		{
//			float ds = 0;
//			for (z=0; z<bpp; z++)
//			{
//				if (z != 3)
//				{
//					float diff = ((float)s[z + (x+1)*bpp + y*bpp*w] - 
//												 (float)s[z + (x-1)*bpp + y*bpp*w]);
//					ds += diff*diff;
//				}
//			}
//			ds = sqrtf(ds);
//			
//			if (ds < 1.0f)
//			{
//				for (z=0; z<bpp; z++)
//				{
//					s[z + (x+0)*bpp + y*bpp*w]
//						= (s[z + (x-1)*bpp + y*bpp*w] + s[z + (x+1)*bpp + y*bpp*w])/2;
//				}
//			}
//		}
		
		//Take derivatives
		for (x=0; x<w; x++)
		{
			deriv[x + y*w] = 0;
			for (z=0; z<bpp; z++)
			{
				if (x== 0 || x == w-1)
					curv[x + y*w] = 0;
				else if (z != 3)
				{
					if (y == 0)
					{
						float diff = ((float)s[z + (x+0)*bpp + y*bpp*w] - 
													 (float)s[z + (x-1)*bpp + y*bpp*w]);
						deriv[x + y*w] += diff*diff;
					}
					else
					{
						float dy1 = (float)s[z + (x+0)*bpp + y*bpp*w] -
									(float)d[z + (x+0)*bpp + (y-1)*bpp*w];
						float dy2 = (float)s[z + (x-1)*bpp + y*bpp*w] -
									(float)d[z + (x-1)*bpp + (y-1)*bpp*w];
						
						if (fabs(dy1) < 6) dy1 = 0;
						if (fabs(dy2) < 6) dy2 = 0;
						float diff = (dy1 - dy2);
						deriv[x + y*w] += diff*diff;
					}
				}
			}
			deriv[x + y*w] = sqrtf(deriv[x + y*w]);
		}
		
		for (x=w-2; x>=1; x--)
		{
			curv[x + y*w] = (deriv[(x+0) + y*w]);
		}
		
		//Loop through and compress a single line
		float cmp[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
		indices[y*w] = 0;
		for (z=0; z<bpp; z++)
		{
			if (y == 0)
				data[y*bpp*w + z] = s[y*bpp*w + z];
			else
				data[y*bpp*w + z] = s[y*bpp*w + z] - d[(y-1)*bpp*w + z];
		}
		
		memUsageReal += sizeof(int) + bpp*sizeof(float);
		mem8bit += sizeof(short) + sizeof(char)*bpp;
		
		int lastNeedToWork = 0;
		int skippedWork = 0;
		int justDidWork = 0;
		int px = 0;
		
		for (x=1; x<w; x++)
		{
			int needToWork = 0;
			float simpleSum = 0;
			
			cmp[0] += (curv[x + y*w]);
			
			if (x < w-1)
				simpleSum += abs(curv[(x+1) + y*w])
						+ abs(curv[(x-1) + y*w]);
			
			if (abs(cmp[0]) > 4.0f)
			{
				needToWork = 1;
			}
			
			if (needToWork == 0 && justDidWork > 1)
				justDidWork--;
			else if (justDidWork == 1 && needToWork == 0)
			{
				justDidWork = -1;
				needToWork = 1;
			}
			
			if (skippedWork)
				needToWork = 1;
			
			if (x == w-1)	needToWork = 1;
			else
			{
				if (x < lastNeedToWork + 0  && needToWork)
				{
					needToWork = 0;
				
					skippedWork = 1;
				}
				else if (needToWork)
					skippedWork = 0;
			}
			
			//if (px >= 255)		needToWork = 1;
			if (needToWork)		lastNeedToWork = x;
			
			if (needToWork)
			{
				int nx = (x == w-1)?x:x-1;
				
				indices[y*w]++;
				int ci = indices[y*w];
				
				indices[y*w+ci] = nx - px;
				px = nx;
				for (z=0; z<bpp; z++)
				{
					if (y == 0)
						data[ci*bpp + y*bpp*w + z] = s[nx*bpp + y*bpp*w + z];
					else
						data[ci*bpp + y*bpp*w + z] = s[nx*bpp + y*bpp*w + z]
													-d[nx*bpp + (y-1)*bpp*w + z];
						
					cmp[z] = 0;
				}
				
				memUsageReal += sizeof(int) + bpp*sizeof(float);
				mem8bit += sizeof(short) + bpp;
				
				/*if (justDidWork == -1)
					justDidWork = 0;
				else
					justDidWork = 3;*/
			}
		}
		
		//Pre-compute results...
		int sc = indices[y*w];
		int prevX = 0;
		
		for (z=0; z<bpp; z++)
			d[z + y*bpp*w] = data[z+y*bpp*w];
		
		for (x=1; x<=sc; x++)
		{
			int curX = indices[y*w+x] + prevX;
			
			int x2;
			for (x2=prevX; x2<=curX; x2++)
			{
				float scaleF = (float)(x2-prevX)/(float)(curX - prevX);
				
				for(z=0;z<bpp;z++)
				{
					if (y == 0)
						d[z + x2*bpp + y*bpp*w] =
								(scaleF)*data[x*bpp + y*bpp*w + z]
								+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z];
					else
						d[z + x2*bpp + y*bpp*w] =
								clamp((scaleF)*data[x*bpp + y*bpp*w + z]
								+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z]
								+ d[z +x2*bpp + (y-1)*bpp*w]);
					
					if (x2 == prevX && z != 3)
					{
						//d[z + x2*bpp + y*bpp*w] = 0;
					}
				}
			}
			
			prevX = curX;
		}
		
	}
	printf("Compressed 32-bit: %ikb\nPotential 16-bit: %ikb\nPotential 8-bit: %ikb\n",
				memUsageReal/1024, memUsageReal/2048, mem8bit/1024);
	
	//Display the results...
	for (y=0; y<h; y++)
	{
		int sc = indices[y*w];
		int prevX = 0;
		
		for (z=0; z<bpp; z++)
			d[z + y*bpp*w] = data[z+y*bpp*w];
		
		for (x=1; x<=sc; x++)
		{
			int curX = indices[y*w+x] + prevX;
			
			int x2;
			for (x2=prevX; x2<=curX; x2++)
			{
				//float scaleF = (float)(x2-prevX)/(float)(curX - prevX);
				
				for(z=0;z<bpp;z++)
				{
					//if (y == 0)
//						d[z + x2*bpp + y*bpp*w] =
//						(scaleF)*data[x*bpp + y*bpp*w + z]
//						+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z];
//					else
//						d[z + x2*bpp + y*bpp*w] =
//						(scaleF)*data[x*bpp + y*bpp*w + z]
//						+ (1-scaleF)*data[(x-1)*bpp + y*bpp*w + z]
//						+ d[z +x2*bpp + (y-1)*bpp*w];
					
					if (x2 == prevX && z != 3)
					{
					//	d[z + x2*bpp + y*bpp*w] = 0;
					}
					else
					{
					//	d[z + x2*bpp + y*bpp*w] = 255;
					}
				}
			}
			
			prevX = curX;
		}
	}
	
	//Display the results...
//	for (y=0; y<h; y++)
//	{
//		for (x=0; x<w; x++)
//		{
//			for (z=0; z<bpp; z++)
//			{
//				d[z + x*bpp + y*bpp*w] = curv[x+y*w];
//				
//				if (z == 3)
//					d[z + x*bpp + y*bpp*w] = 255;
//			}
//		}
//	}
	
	/*for (y=0; y<h; y++)
	{
		for(x=0; x<w; x++)
		{
			for (z=0; z<bpp; z++)
			{
				if (x == 0)
					d[z + x*bpp + y*bpp*w] = s[z + x*bpp + y*bpp*w];
				else
					d[z + x*bpp + y*bpp*w] = (s[z + x*bpp + y*bpp*w] - 
											 s[z + (x-1)*bpp + y*bpp*w]) + 127;
			}
		}
	}
	
	for (y=0; y<h; y++)
	{
		for(x=w-1; x>=1; x--)
		{
			for (z=0; z<bpp; z++)
			{
				if (z == 1)
					d[z + x*bpp + y*bpp*w] = (d[z + x*bpp + y*bpp*w] - 
									d[z + (x-1)*bpp + y*bpp*w]) + 127;
				else
					d[z + x*bpp + y*bpp*w] = 0;
			}
		}
	}*/
	
	[NSBundle loadNibNamed:@"FluidZip_Window" owner:self];
	[r_windows addObject:i_window];
	[i_window release];
	
	[i_window setImage:i2];
	[i_window setTitle:[[m_focus title] stringByAppendingString:@".vec"]];
	[i_window setDelegate:self];
	
	free(indices);
	free(data);
	free(curv);
	free(deriv);
	
	[i2 release];
	[cpy release];
	[bir release];
}


- (void)onGotFocus:(FluidZip_Window*)in_window
{
	m_focus = in_window;
}

@end