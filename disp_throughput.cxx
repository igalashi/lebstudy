/*
 *
 */

double mean_array(int *data, int ndata)
{
	long sum = 0;
	long entry = 0;
	for (int i = 0 ; i < ndata ; i++) {
		sum += (i * data[i]);
		entry += data[i];
	}
	double mean = static_cast<double>(sum) / static_cast<double>(entry);

	return mean;
}

void disp_throughput(void *cdata, int data_size)
{
	static long int wcount = 0;
	static mStopWatch sw;
	static bool at_start = true;
	static int stat_flag[2] = {0, 0};
	static int stat_nodes[110];

	struct packed_event *pheader;
	pheader = reinterpret_cast<struct packed_event *>(cdata);


	if (at_start) {
		at_start = false;
		sw.start();

		for (int i = 0 ; i < 110 ; i++) stat_nodes[i] = 0;
	}


	unsigned int flag = ntohl(pheader->flag);
	unsigned int nodes = ntohs(pheader->nodes);
	if (flag == 1) {
		stat_flag[1]++;
	} else {
		stat_flag[0]++;
	}
	stat_nodes[nodes]++;

	#if 0
	double flag_ratio = static_cast<double>(stat_flag[1]) /
			static_cast<double>(stat_flag[0] + stat_flag[1]);
	std::cout << "#D magic: 0x" << std::hex << ntohl(pheader->magic)
		<< " len: " << std::dec << std::setw(8) << ntohl(pheader->length)
		<< " id: " << std::setw(4) << ntohs(pheader->id)
		<< " nodes: " << std::setw(3) << nodes
		<< " flag: " << flag
		<< " Su eff. : " << flag_ratio
		<< std::endl;
	#endif

	wcount += data_size;
	static unsigned int ebcount = 0;
	if ((ebcount % 100) == 0) {
		int elapse = sw.elapse();
		if (elapse >= 10000) {
			sw.start();
			double wspeed = static_cast<double>(wcount)
				/ 1024 / 1024
				* 1000 / static_cast<double>(elapse);
			double freq = ebcount * 1000 / static_cast<double>(elapse);

			double flag_ratio = static_cast<double>(stat_flag[1]) /
				static_cast<double>(stat_flag[0] + stat_flag[1]);
			std::cout << "# Freq: " << freq << " Throughput: "
			<< wcount << " B " << elapse << " ms "
			<< wspeed << " MiB/s"
			<< " Su Eff.: " << flag_ratio
			<< " nodes: " << mean_array(stat_nodes, 105)
			<< std::endl;

			wcount = 0;
			ebcount = 0;

			stat_flag[0] = 0;
			stat_flag[1] = 0;
			for (int i = 0 ; i < 110 ; i++) stat_nodes[i] = 0;
		}
	}
	ebcount++;

	return;
}
