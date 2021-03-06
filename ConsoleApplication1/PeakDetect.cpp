// ConsoleApplication1.cpp: 콘솔 응용 프로그램의 진입점을 정의합니다.
//

#include "stdafx.h"

#include <math.h>

/*
* Copyright 2011, 2013 Hong Xu. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*
*   2. Redistributions in binary form must reproduce the above copyright
*      notice, this list of conditions and the following disclaimer in the
*      documentation and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY HONG XU ``AS IS'' AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
* EVENT SHALL HONG XU OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
* THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
* The views and conclusions contained in the software and documentation are
* those of the authors and should not be interpreted as representing official
* policies, either expressed or implied, of Hong Xu.
*
*
*
* Purpose: Peak detection in a wave.
*
* Author: Hong Xu <hong@hong.me>
*
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_help(void)
{
	fprintf(stderr,
		"Usage: peakdetect [OPTIONS]\n"
		"Peak detection in a wave\n"
		"\n"
		"Options:\n"
		"-i inputfile \t\tInput file.\n"
		"             \t\tThe input file should be a csv format file, "
		"whose first\n"
		"             \t\tcolumn is X and second column is Y.\n"
		"-o outfile   \t\tOutput file.\n"
		"             \t\tEmission peaks will be output first, "
		"followed by\n"
		"             \t\tabsorption peaks with an empty line seperated."
		"\n"
		"-d deltavalue\t\tDelta, a parameter used to determine peaks.\n"
		"-m mode      \t\tDetecting mode, "
		"could be either \"a\" (detect absorption peak\n"
		"             \t\tfirst) or \"e\" (detect emission peak first).\n"
		"             \t\tDefault value is \"a\".\n"
		"--version    \t\tDisplay version information.\n"
		"--help       \t\tShow this help information.\n"
		"\n"
		"e.g.\n"
		"peakdetect -i input.csv -o output.csv -d 1e-7 -m a\n"
		"peakdetect <input.csv -d 0.1 -m e | tee out.csv\n");
	exit(0);
}

void print_version(void)
{
	fprintf(stderr,
		"peakdetect version 0.1.3\n"
		"Copyright (C) 2011 Hong Xu <xuphys@gmail.com>\n"
		"Originally inspired by Eli Billauer\'s peakdet for MATLAB:\n"
		"http://billauer.co.il/peakdet.html\n"
		"\n"
		"See the README file for license information.\n");
	exit(0);
}

int detect_peak(
	const double*   data, /* the data */
	int             data_count, /* row count of data */
	int*            emi_peaks, /* emission peaks will be put here */
	int*            num_emi_peaks, /* number of emission peaks found */
	int             max_emi_peaks, /* maximum number of emission peaks */
	int*            absop_peaks, /* absorption peaks will be put here */
	int*            num_absop_peaks, /* number of absorption peaks found */
	int             max_absop_peaks, /* maximum number of absorption peaks
									 */
	double          delta, /* delta used for distinguishing peaks */
	int             emi_first /* should we search emission peak first of
							  absorption peak first? */
)
{
	int     i;
	double  mx;
	double  mn;
	int     mx_pos = 0;
	int     mn_pos = 0;
	int     is_detecting_emi = emi_first;


	mx = data[0];
	mn = data[0];

	*num_emi_peaks = 0;
	*num_absop_peaks = 0;

	for (i = 1; i < data_count; ++i)
	{
		if (data[i] > mx)
		{
			mx_pos = i;
			mx = data[i];
		}
		if (data[i] < mn)
		{
			mn_pos = i;
			mn = data[i];
		}

		if (is_detecting_emi &&
			data[i] < mx - delta)
		{
			if (*num_emi_peaks >= max_emi_peaks) /* not enough spaces */
				return 1;

			emi_peaks[*num_emi_peaks] = mx_pos;
			++(*num_emi_peaks);

			is_detecting_emi = 0;

			i = mx_pos - 1;

			mn = data[mx_pos];
			mn_pos = mx_pos;
		}
		else if ((!is_detecting_emi) &&
			data[i] > mn + delta)
		{
			if (*num_absop_peaks >= max_absop_peaks)
				return 2;

			absop_peaks[*num_absop_peaks] = mn_pos;
			++(*num_absop_peaks);

			is_detecting_emi = 1;

			i = mn_pos - 1;

			mx = data[mn_pos];
			mx_pos = mn_pos;
		}
	}

	return 0;
}

int main(int argc, const char *argv[])
{
#define INITIAL_ROW_COUNT       1500
#define ROW_COUNT_INCREASEMENT  3000
	double*     data[5];
	double      row[2];
#define MAX_PEAK    200
	int         emi_peaks[MAX_PEAK];
	int         absorp_peaks[MAX_PEAK];
	int         emi_count = 0;
	int         absorp_count = 0;
#define LINE_BUFFER_SIZE    120
	char        line[LINE_BUFFER_SIZE];
	FILE*       out = stdout;
	FILE*       in = stdin;
	int         i;
	double      delta = 1e-6;
	int         emission_first = 0;
	int         idummy;

	/*
	* argument parsing
	*/
	{
		int flag_delta = 0;
		int flag_in = 0;
		int flag_out = 0;
		int flag_mode = 0;
		for (i = 1; i < argc; ++i)
		{
			if (flag_delta)
			{
				delta = atof(argv[i]);
				flag_delta = 0;
			}
			else if (flag_in)
			{
				errno_t err = fopen_s(&in, argv[i], "r");
				if (!in)
				{
					fprintf(stderr, "Failed to open file \"");
					fprintf(stderr, argv[i]);
					fprintf(stderr, "\".\n");
					exit(2);
				}
				flag_in = 0;
			}
			else if (flag_out)
			{
				errno_t err = fopen_s(&out, argv[i], "w");
				if (!out)
				{
					fprintf(stderr, "Failed to open file \"");
					fprintf(stderr, argv[i]);
					fprintf(stderr, "\".\n");
					exit(2);
				}
				flag_out = 0;

			}
			else if (flag_mode)
			{
				if (!strcmp(argv[i], "a"))
					emission_first = 0;
				else if (!strcmp(argv[i], "e"))
					emission_first = 1;
				else
				{
					fprintf(stderr,
						"Argument parsing error: Unknown mode \"");
					fprintf(stderr, argv[i]);
					fprintf(stderr, "\"\n");
					exit(4);
				}

				flag_mode = 0;
			}
			else if (!strcmp(argv[i], "-d"))
				flag_delta = 1;
			else if (!strcmp(argv[i], "-i"))
				flag_in = 1;
			else if (!strcmp(argv[i], "-o"))
				flag_out = 1;
			else if (!strcmp(argv[i], "-m"))
				flag_mode = 1;
			else if (!strcmp(argv[i], "--help"))
				print_help();
			else if (!strcmp(argv[i], "--version"))
				print_version();
			else
			{
				fprintf(stderr, "Unknown option \"");
				fprintf(stderr, argv[i]);
				fprintf(stderr, "\".\n");
				exit(3);
			}

		}
	}

	data[0] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT);
	data[1] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT);
	data[2] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT); //for DC level
	data[3] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT); //for AC level
	data[4] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT); //for DC to AC ratio(%)
	//data[5] = (double*)malloc(sizeof(double) * INITIAL_ROW_COUNT); //for AC Peak to Peak

	/* read data */
	i = 0;
	int iValidCnt = 0;
	while (fgets(line, LINE_BUFFER_SIZE, in))
	{
		/* when the buffer is not large enough, increase the buffer size */
		idummy = i - INITIAL_ROW_COUNT;
		if (idummy >= 0 && idummy % ROW_COUNT_INCREASEMENT == 0)
		{
			double*     tmp;
			int         j;

			for (j = 0; j < 2; ++j)
			{
				tmp = (double*)malloc(
					sizeof(double) * (i + ROW_COUNT_INCREASEMENT));
				memcpy(tmp, data[j], i * sizeof(double));
				free(data[j]);
				data[j] = tmp;
			}
		}

		sscanf_s(line, "%lf,%lf", row, row + 1);
		data[0][i] = row[0];
		data[1][i] = row[1];
		++i;
	}
	iValidCnt = i;
	if (detect_peak(data[1], i, emi_peaks, &emi_count, MAX_PEAK, absorp_peaks, &absorp_count, MAX_PEAK, delta, emission_first))
	{
		fprintf(stderr, "There are too many peaks.\n");
		exit(1);
	}

	for (i = 0; i < iValidCnt; i++)
	{
		data[2][i] = data[1][i];
	}

	double fTemp = 0.0f;
	double fACPeakToPeakAverage = 0.0f;
	for (int i = 0; i < absorp_count; i++)
	{
		if(i< absorp_count - 1)
		{
			for (int j = absorp_peaks[i]; j < absorp_peaks[i + 1]; j++)
			{
				fTemp += data[1][j];
			}

			fTemp = fTemp / (absorp_peaks[i + 1] - absorp_peaks[i]);


			for (int j = absorp_peaks[i]; j < absorp_peaks[i + 1]; j++)
			{
				data[2][j] = fTemp;
			}

			if (i == 0)
			{
				for (int j = 0; j < absorp_peaks[i]; j++)
				{
					data[2][j] = fTemp;
				}
			}
		}
		else
		{

			if (absorp_count > emi_count)
			{
				for (int j = absorp_peaks[i]; j < iValidCnt; j++)
				{
					data[2][j] = data[2][j-1];
				}
			}
			else
			{
				for (int j = absorp_peaks[i]; j < emi_peaks[i]; j++)
				{
					fTemp += data[1][j];
				}

				fTemp = fTemp / (emi_peaks[i] - absorp_peaks[i]);


				for (int j = absorp_peaks[i]; j < iValidCnt; j++)
				{
					data[2][j] = fTemp;
				}
			}
		}

		fTemp = 0.0f;

	}

	for (i = 0; i < iValidCnt; i++)
	{
		data[3][i] = data[1][i] - data[2][i]; //AC Level
		data[4][i] = data[3][i] * 100.0f / data[2][i];
	}

	fprintf(out, "Index(20ms),Raw,DC Level,AC Level,AC/DC ratio(%%),peaks\n");
	for (i = 0; i < iValidCnt; i++)
	{
		fprintf(out, "%lf,%lf,%lf,%lf,%lf,", data[0][i], data[1][i], data[2][i], data[3][i], data[4][i]);
		for (int j = 0; j < MAX_PEAK; j++)
		{
			if (emi_peaks[j] == i)
			{
				if (j > 0)
				{
					fprintf(out, "%lf,local max, %d", data[1][emi_peaks[j]] - data[1][absorp_peaks[j]], j);
				}
				else
				{
					fprintf(out, "%lf,1st max", data[1][emi_peaks[j]]);
				}
				break;
			}
		}
		for (int j = 0; j < MAX_PEAK; j++)
		{
			if (absorp_peaks[j] == i)
			{
				if (j > 0)
				{
					fprintf(out, "%lf,local min,%d", data[1][emi_peaks[j - 1]] - data[1][absorp_peaks[j]], j);
				}
				else
				{
					fprintf(out, "%lf,1st min", data[1][absorp_peaks[j]]);
				}
				
				break;
			}
		}
		fprintf(out, "\n");

	}

	

	fTemp = 0.0f;
	for (i = 0; i < absorp_count; ++i)
	{
		fTemp += fabs(data[4][absorp_peaks[i]]);
	}

	for (i = 0; i < emi_count; ++i)
	{
		fTemp += fabs(data[4][emi_peaks[i]]);
	}

	//for (i = 0; i < emi_count; ++i)
	//{
	//	data[5][i] = data[3][emi_peaks[i]] - data[3][absorp_peaks[i]]; //AC Peak to Peak
	//}

	double fACPeakToPeak[1000] = { 0.0f };

	for (i = 0; i < emi_count; ++i)
	{
		fACPeakToPeak[2 * i] = fabs(i == 0 ? fabs(data[3][absorp_peaks[i]]) : data[3][emi_peaks[i - 1]] - data[3][absorp_peaks[i]]);
		fACPeakToPeakAverage += fACPeakToPeak[2 * i];
		fACPeakToPeak[2 * i + 1] = fabs(data[3][emi_peaks[i]] - data[3][absorp_peaks[i]]);
		fACPeakToPeakAverage += fACPeakToPeak[2 * i + 1];
	}

	fTemp = fTemp / (absorp_count + emi_count);
#if 0
	fprintf(out, "\nPeak Index, Peak,DC Level,AC Level,AC/DC Ratio(%%),Absolute AC/DC Ratio(%%), AC/DC Average Ratio(%lf%%),AC Peak to Peak, AC(Peak to Peak) Average=%.2lf, Reference(0.1%%)\n", fTemp, fACPeakToPeakAverage / (2 * emi_count));

	for (i = 0; i < emi_count; ++i)
	{
		fprintf(out, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%.2lf%%,0.1%%\n",
			data[0][absorp_peaks[i]],
			data[1][absorp_peaks[i]],
			data[2][absorp_peaks[i]],
			data[3][absorp_peaks[i]],
			data[4][absorp_peaks[i]],
			fabs(data[4][absorp_peaks[i]]), 
			fTemp,
			fACPeakToPeak[2 * i],
			fACPeakToPeak[2 * i] * 100 / data[2][absorp_peaks[i]]
		);
		fprintf(out, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%.2lf%%,0.1%%\n",
			data[0][emi_peaks[i]],
			data[1][emi_peaks[i]],
			data[2][emi_peaks[i]],
			data[3][emi_peaks[i]],
			data[4][emi_peaks[i]], 
			fabs(data[4][emi_peaks[i]]),
			fTemp,
			fACPeakToPeak[2 * i + 1],
			fACPeakToPeak[2 * i + 1] * 100 / data[2][emi_peaks[i]]
			);
	}
#else

	

	float fACDCAverageRatio = 0.0f;
	for (i = 0; i < emi_count; ++i)
	{
		fACDCAverageRatio += fACPeakToPeak[2 * i] * 100 / data[2][absorp_peaks[i]];
		fACDCAverageRatio += fACPeakToPeak[2 * i + 1] * 100 / data[2][emi_peaks[i]];
	}

	fACDCAverageRatio = fACDCAverageRatio / (2 * emi_count);

	fprintf(out, "\nPROJET:BioWatch");
	fprintf(out, "\nDetect Peaks %d", emi_count+ absorp_count);
	fprintf(out, "\nPeak Index, Peak,DC Level,AC Level, AC(Peak to Peak) Average=%.2lf, AC(Peak to Peak)/DC Ratio(%%),  AC/DC Average Ratio(%.2lf%%), Reference(0.1%%)\n", fACPeakToPeakAverage / (2 * emi_count), fACDCAverageRatio);

	for (i = 0; i < emi_count; ++i)
	{
		fprintf(out, "%lf,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf%%,%.2lf%%,0.1%%\n",
			data[0][absorp_peaks[i]],
			data[1][absorp_peaks[i]],
			data[2][absorp_peaks[i]],
			data[3][absorp_peaks[i]],
			//data[4][absorp_peaks[i]],
			//fabs(data[4][absorp_peaks[i]]),
			//fTemp,
			fACPeakToPeak[2 * i],
			fACPeakToPeak[2 * i] * 100 / data[2][absorp_peaks[i]],
			fACDCAverageRatio
		);
		fprintf(out, "%lf,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf%%,%.2lf%%,0.1%%\n",
			data[0][emi_peaks[i]],
			data[1][emi_peaks[i]],
			data[2][emi_peaks[i]],
			data[3][emi_peaks[i]],
			//data[4][emi_peaks[i]],
			//fabs(data[4][emi_peaks[i]]),
			//fTemp,
			fACPeakToPeak[2 * i + 1],
			fACPeakToPeak[2 * i + 1] * 100 / data[2][emi_peaks[i]],
			fACDCAverageRatio
		);
	}

#endif
	//for (i = 0; i < absorp_count; ++i)fprintf(out, "%lf,%lf\n", data[0][absorp_peaks[i]], data[1][absorp_peaks[i]]);

	return 0;
}
