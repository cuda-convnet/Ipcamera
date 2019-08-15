/* rate control */
//#include "ratecontrol.h"
#include "ratecontrol-mpeg4.h"

#define FRAMERATE_INCR		1001
#define SMALL_EPS 1e-10

//++ Foster
//Marked By Ken.Hsieh
//extern int fc_debug;
int fc_debug=0;
#define DBG_MSG(fmt,args...) if(fc_debug) printf(fmt , ## args)
//-- Foster

int MP4_get_framerate(float fFrameRate)
{
    int fincr,fbase,i;

	if ((fFrameRate - (int)fFrameRate) < SMALL_EPS) {
		fincr = 1;
		fbase = (int)fFrameRate;
	}
	else {
		fincr = FRAMERATE_INCR;
		fbase = (int)(FRAMERATE_INCR * fFrameRate);
	}
	if (fincr <= 0 || fbase <= 0) {
		fincr = 1;
		fbase = 25;
	}
	i = fincr; //i = pParam->fincr;
	while (i > 1) {
		if (fincr % i == 0 && fbase % i == 0) {
			fincr /= i;
			fbase /= i;
			i = fincr;
			continue;
		}
		i--;
	}
	if (fbase > 65535) {
		float div = (float) fbase / 65535;
		fbase = (int) (fbase / div);
		fincr = (int) (fincr / div);
	}
	if (fbase > 65535) {
		float div = (float) fbase / 65535;
		fbase = (int) (fbase / div);
		fincr = (int) (fincr / div);
	}   
	//printf("get_framerate =%d\n",fbase*1000/fincr);
	return (fbase*1000/fincr);
}


void MP4_RateControlInit(RateControl * rate_control,
				unsigned int target_rate,
				unsigned int reaction_delay_factor,
				unsigned int averaging_period,
				unsigned int buffer,
				int framerate,
				int max_quant,
				int min_quant,
				unsigned int initq)
{
	int i;
	
//++ Foster
DBG_MSG("%s:%d <bit=%d:%d, fps=%d, Q=%d\n>",__FUNCTION__,__LINE__,
									rate_control->target_rate,
									target_rate,
									framerate,
									initq);
//-- Foster
	rate_control->frames = 0;
	rate_control->total_size = 0;
	rate_control->framerate = (double) framerate / 1000.0;
//	printf("framerate=%f\n",  rate_control->framerate);
	rate_control->target_rate = (int) target_rate;
//	rate_control->rtn_quant = get_initial_quant(target_rate);
	rate_control->rtn_quant = initq;
	rate_control->max_quant = (short) max_quant;
	rate_control->min_quant = (short) min_quant;
	for (i = 0; i < 32; ++i) {
		rate_control->quant_error[i] = 0.0;
	}
	rate_control->target_framesize =
		(double) target_rate / 8.0 / rate_control->framerate;
	//printf("rate_control->target_framesize =%f\n", rate_control->target_framesize);	
	rate_control->sequence_quality = 2.0 / (double) rate_control->rtn_quant;
//rate_control->sequence_quality =0.250000
	
	rate_control->avg_framesize = rate_control->target_framesize;
	rate_control->reaction_delay_factor = (int) reaction_delay_factor;
	rate_control->averaging_period = (int) averaging_period;
	rate_control->buffer = (int) buffer;
}


void MP4_RateControlUpdate(RateControl *rate_control,
				  short quant,
				  int frame_size,
				  int keyframe)
{
	long long deviation;
	double overflow, averaging_period, reaction_delay_factor;
	double quality_scale, base_quality, target_quality;
	int rtn_quant;

	rate_control->frames++;
	rate_control->total_size += frame_size;

	deviation =
		(long long) ((double) rate_control->total_size -
				   ((double) ((double) rate_control->target_rate / 8.0 /
					 (double) rate_control->framerate) * (double) rate_control->frames));
//printf("deviation =%u\n", deviation);	
/*deviation =83825
deviation =213595
deviation =213233
deviation =211783
deviation =215849
deviation =210171
deviation =207269
deviation =209982
deviation =206073
deviation =207988
*/
	if (rate_control->rtn_quant >= 2) {
		averaging_period = (double) rate_control->averaging_period;
/*		printf("averaging_period =%f\n", averaging_period);	
averaging_period =100.000000
averaging_period =100.000000
averaging_period =100.000000
averaging_period =100.000000
*/
		rate_control->sequence_quality -=
			rate_control->sequence_quality / averaging_period;
/*printf("rate_control->sequence_quality =%f\n", rate_control->sequence_quality);	
rate_control->sequence_quality =0.279320
rate_control->sequence_quality =0.279827
rate_control->sequence_quality =0.280328
rate_control->sequence_quality =0.280825
rate_control->sequence_quality =0.281317
rate_control->sequence_quality =0.281804
rate_control->sequence_quality =0.282946
rate_control->sequence_quality =0.283416
rate_control->sequence_quality =0.284542
rate_control->sequence_quality =0.285657
rate_control->sequence_quality =0.286100
rate_control->sequence_quality =0.287199
*/
		rate_control->sequence_quality +=
			2.0 / (double) rate_control->rtn_quant / averaging_period;
		if (rate_control->sequence_quality < 0.1)
			rate_control->sequence_quality = 0.1;

		if (!keyframe) {
			reaction_delay_factor =
				(double) rate_control->reaction_delay_factor;
			rate_control->avg_framesize -=
				rate_control->avg_framesize / reaction_delay_factor;
			rate_control->avg_framesize += frame_size / reaction_delay_factor;
		}
	}

	quality_scale =
		rate_control->target_framesize / rate_control->avg_framesize *
		rate_control->target_framesize / rate_control->avg_framesize;

	base_quality = rate_control->sequence_quality;
	if (quality_scale >= 1.0) {
		base_quality = 1.0 - (1.0 - base_quality) / quality_scale;
	} else {
		base_quality = 0.06452 + (base_quality - 0.06452) * quality_scale;
	}

	overflow = -((double) deviation / (double) rate_control->buffer);

	target_quality =
		base_quality + (base_quality -
						0.06452) * overflow / rate_control->target_framesize;

	if (target_quality > 2.0)
		target_quality = 2.0;
	else if (target_quality < 0.06452)
		target_quality = 0.06452;

	rtn_quant = (int) (2.0 / target_quality);

	if (rtn_quant < 31) {
		rate_control->quant_error[rtn_quant] +=
			2.0 / target_quality - rtn_quant;
		if (rate_control->quant_error[rtn_quant] >= 1.0) {
			rate_control->quant_error[rtn_quant] -= 1.0;
			rtn_quant++;
		}
	}

	if (rtn_quant > rate_control->rtn_quant + 1)
		rtn_quant = rate_control->rtn_quant + 1;
	else if (rtn_quant < rate_control->rtn_quant - 1)
		rtn_quant = rate_control->rtn_quant - 1;

	if (rtn_quant > rate_control->max_quant)
		rtn_quant = rate_control->max_quant;
	else if (rtn_quant < rate_control->min_quant)
		rtn_quant = rate_control->min_quant;
	rate_control->rtn_quant = rtn_quant;
}
