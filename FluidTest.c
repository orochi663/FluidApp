/*
 *  FluidTest.c
 *  FluidApp */


//Simple application that does testing of the code.  This ensures that there
//are no major errors within the program.  Also, a place that can be used to
//check for regression tests.

//Essentially a set of unit tests...

void testHalfFloat();
void testLocality();
void testPipeline();

int main(int argc, char **argv)
{
	//testHalfFloat();
//	testLocality();
//	testLocality();
//	testLocality();
//	testLocality();

	testPipeline();
	
	return 0;
}
