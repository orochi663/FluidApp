//
//  FluidFPSView.m
//  FluidApp
//

#import "FluidFPSView.h"


@implementation FluidFPSView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
        m_curSample = 0;
		int v;
		for (v=0; v<MAXFPSVIEWS; v++)
		{
			int s;
			for (s=0; s<MAXFPSSAMPLES; s++)
			{
				m_samples[v][s] = nil;
			}
			m_titles[v] = nil;
			m_colors[v] = nil;
		}
    }
    return self;
}

- (void)drawRect:(NSRect)dirtyRect
{
    //Clear to white...
	[[NSColor whiteColor] set];
	[NSBezierPath fillRect:dirtyRect];
	
	//Draw grey lines for speed of simulation.
	//	1 line for each 0.1ms
	NSRect rect = [self bounds];
	float x;
	[[NSColor grayColor] set];
	NSDictionary *fontAttributes =
				[NSDictionary dictionaryWithObject:[NSColor grayColor]
								forKey:NSForegroundColorAttributeName];
	NSDictionary *fontWhite =
			[NSDictionary dictionaryWithObject:[NSColor whiteColor]
								forKey:NSForegroundColorAttributeName];
	for (x=0; x<10; x++)
	{
		float y = (x+1)*rect.size.height/11.0f + rect.origin.y;
		NSPoint p1 = {rect.origin.x, y};
		NSPoint p2 = {rect.origin.x + rect.size.width, y};
		
		[NSBezierPath strokeLineFromPoint:p1 toPoint:p2];
		
		p1.x+=2.0f;
		p1.y+=2.1;
		[[NSString stringWithFormat:@"%2.2fms", x/30.0f]
		 drawAtPoint:p1 withAttributes:fontAttributes];
		
		p2.x-=56;
		p2.y+=2.1;
		[[NSString stringWithFormat:@"%2.2f fps", 1.0f/(x/30.0f)]
			 drawAtPoint:p2 withAttributes:fontAttributes];
	}
	
	int v;
	
	float curY = rect.size.height + rect.origin.y;
	float avg;
	
	for (v=0; v<MAXFPSVIEWS; v++)
	{
		if (m_titles[v] != nil && m_colors[v] != nil)
		{
			[NSBezierPath setDefaultLineWidth:2.0f];
			
			avg = 0;
			int s;
			if (m_enabled[v])
			{
				[m_colors[v] set];
				for (s=0; s<MAXFPSSAMPLES-2; s++)
				{
					float fs = (float)s;
					
					float s1 = m_samples[v][(s+m_curSample)%MAXFPSSAMPLES];
					float s2 = m_samples[v][(s+1+m_curSample)%MAXFPSSAMPLES];
					avg+=s1;
					
					float rm1 = 1*rect.size.height/11.0f;
					
					s1=s1*30*rect.size.height/11+rm1;
					s2=s2*30*rect.size.height/11+rm1;
					
					NSPoint p1 = {rect.origin.x + fs*rect.size.width/(MAXFPSSAMPLES-1),
									s1};
					NSPoint p2 = {rect.origin.x + (fs+1)*rect.size.width/(MAXFPSSAMPLES-1),
									s2};
					
					[NSBezierPath strokeLineFromPoint:p1 toPoint:p2];
				}
			}
			else
			{
				[[NSColor blackColor] set];
				for (s=0; s<MAXFPSSAMPLES-2; s++)
				{
					float s1 = m_samples[v][(s+m_curSample)%MAXFPSSAMPLES];
					avg+=s1;
				}
			}
			avg = (avg / MAXFPSSAMPLES);
			
			NSRect rc = {rect.origin.x + rect.size.width/2-64, curY-12,
						128, 12};
			[NSBezierPath fillRect:rc];
			
			m_regions[v] = rc;
			
			rc.origin.x += 2;
			rc.origin.y -= 2;
			[[NSString stringWithFormat:@"%@ %2.2f ms",m_titles[v],  avg]
			 drawAtPoint:rc.origin withAttributes:fontWhite];
			
			curY -= 14;
		}
	}
}

- (void)mouseDown:(NSEvent *)theEvent
{
	NSPoint pt = [self convertPoint:[theEvent locationInWindow] fromView:nil];
	
	int v;
	for (v=0; v<MAXFPSVIEWS; v++)
	{
		if (m_titles[v] != nil && m_colors[v] != nil)
		{			
			if (NSPointInRect(pt, m_regions[v]))
				m_enabled[v] = !m_enabled[v];
		}
	}
}

- (BOOL)isOpaque	{	return YES;		}

- (BOOL)visible
{
	return ([self frame].size.width > 0);
}

- (void)tick
{
	m_curSample++;
	if (m_curSample > MAXFPSSAMPLES)
		m_curSample = 0;
	
	if ([self frame].size.width <= 0)	return;
	
	[self setNeedsDisplay:YES];
}

- (void)setTitle:(NSString*)in_title forIndex:(int)in_index
{
	if (in_title != nil)
		[in_title retain];
		
	if (m_titles[in_index] != nil)
		[m_titles[in_index] release];
	
	m_titles[in_index] = in_title;
}

- (void)setColor:(NSColor*)in_title forIndex:(int)in_index
{
	if (in_title != nil)
		[in_title retain];
	
	if (m_colors[in_index] != nil)
		[m_colors[in_index] release];
	
	m_colors[in_index] = in_title;
}

- (void)setSample:(float)in_sample forIndex:(int)in_index
{
	m_samples[in_index][m_curSample] = in_sample;
}

@end
