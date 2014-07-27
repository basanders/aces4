#include "gtest/gtest.h"
#include <fenv.h>
#include <execinfo.h>
#include <signal.h>
#include <cstdlib>
#include "siox_reader.h"
#include "io_utils.h"
#include "setup_reader.h"

#include "sip_tables.h"
#include "interpreter.h"
#include "setup_interface.h"
#include "sip_interface.h"
#include "data_manager.h"
#include "global_state.h"
#include "sial_printer.h"

#include "worker_persistent_array_manager.h"
#include "server_persistent_array_manager.h"

#include "block.h"

#ifdef HAVE_TAU
#include <TAU.h>
#endif

#ifdef HAVE_MPI
#include "sip_server.h"
#include "sip_mpi_attr.h"
#include "global_state.h"
#include "sip_mpi_utils.h"
#else
#include "sip_attr.h"
#endif


//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);



//bool VERBOSE_TEST = false;
bool VERBOSE_TEST = true;

void list_block_map();
static const std::string dir_name("src/sialx/test/");

#ifdef HAVE_MPI
sip::SIPMPIAttr *attr = &sip::SIPMPIAttr::get_instance();
void barrier() {
	sip::SIPMPIUtils::check_err (MPI_Barrier(MPI_COMM_WORLD));}
#else
	sip::SIPAttr *attr = &sip::SIPAttr::get_instance();
	void barrier() {}
#endif


class TestController {
public:
	TestController(std::string job, bool has_dot_dat_file, bool verbose, std::string comment, std::ostream& sial_output):
	job_(job),
	verbose_(verbose),
	comment_(comment),
	sial_output_(sial_output),
	sip_tables_(NULL),
	wpam_(NULL),
#ifdef HAVE_MPI
	spam_(NULL),
	server_(NULL),
#endif
	worker_(NULL) {
		printer_ = new sip::SialPrinterForTests(sial_output_, attr->global_rank());
		if (has_dot_dat_file){
			setup::BinaryInputFile setup_file(job + ".dat");
			setup_reader_ = new setup::SetupReader(setup_file);
		}
		else{
			setup_reader_= setup::SetupReader::get_empty_reader();
		}
		if (verbose) {
			std::cout << "**************** STARTING TEST " << job_
			<< " ***********************!!!\n"
			<< std::flush;
		}
	}



	~TestController() {
		if (setup_reader_)
			delete setup_reader_;
		if (sip_tables_)
			delete sip_tables_;
		if (wpam_)
			delete wpam_;
#ifdef HAVE_MPI
		if (spam_)
			delete spam_;
		if (server_)
			delete server_;
#endif
		if (worker_)
			delete worker_;
		if (verbose_)
			std::cout << "\nRank " << attr->global_rank() << " TEST " << job_
					<< " TERMINATED" << std::endl << std::flush;
		if (printer_) delete printer_;
	}
	const std::string job_;
	const std::string comment_;
	bool verbose_;
	setup::SetupReader* setup_reader_;
	sip::SipTables* sip_tables_;
	sip::WorkerPersistentArrayManager* wpam_;
#ifdef HAVE_MPI
	sip::ServerPersistentArrayManager* spam_;
	sip::SIPServer* server_;
#endif
	sip::Interpreter* worker_;
	std::ostream& sial_output_;
	sip::SialPrinterForTests* printer_;

	sip::IntTable* int_table(){return &(sip_tables_->int_table_);}

	int int_value(const std::string& name){
		try{
			return worker_->data_manager_.int_value(name);
		}
		catch(std::exception& e){
			std::cerr << "FAILURE: " << name << " not found in int map.  This is probably a bug in the test." << std::endl << std::flush;
			ADD_FAILURE();
			return -1;
		}
	}

	double scalar_value(const std::string& name){
		try{
			return worker_->data_manager_.scalar_value(name);
		}
		catch(std::exception& e){
			std::cerr << "FAILURE: " << name << " not found in scalar map.  This is probably a bug in the test." << std::endl << std::flush;
			ADD_FAILURE();
			return -1;
		}
	}

	void initSipTables() {
		std::string prog_name = job_ + ".siox";
		std::string siox_dir(dir_name);
		setup::BinaryInputFile siox_file(siox_dir + prog_name);
		sip_tables_ = new sip::SipTables(*setup_reader_, siox_file);
		if (verbose_) {
			//rank 0 prints and .siox files contents
			if (attr->global_rank() == 0) {
				std::cout << "JOBNAME = " << job_ << std::endl << std::flush;
				std::cout << "SETUP READER DATA:\n" << *setup_reader_
						<< std::endl << std::flush;
				std::cout << "SIP TABLES" << '\n' << *sip_tables_ << std::endl
						<< std::flush;
				std::cout << comment_ << std::endl << std::flush;
			}
		}
	}
	void runWorker() {

		worker_ = new sip::Interpreter(*sip_tables_, printer_);
		barrier();
		if (VERBOSE_TEST)
			std::cout << "Rank " << attr->global_rank() << " SIAL PROGRAM "
					<< job_ << " STARTING" << std::endl << std::flush;
		try{
		worker_->interpret();
		}
		catch (std::exception& e){
			std::cerr << "exception thrown in worker: " << e.what();
			FAIL();
		}
		if (verbose_)
			if (std::cout != sial_output_) std::cout << sial_output_.rdbuf();
			std::cout << "\nRank " << attr->global_rank() << " SIAL PROGRAM "
					<< job_ << " TERMINATED" << std::endl << std::flush;
		barrier();
	}

};


TEST(BasicSial,DISABLED_empty0){
	TestController controller("empty", false, VERBOSE_TEST, "this is a an empty program using the test controller", std::cout);
	controller.initSipTables();
	controller.runWorker();
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
}

TEST(BasicSial,DISABLED_helloworld){
	TestController controller("helloworld", false, true, "this test should print \"hello world\"", std::cout);
	controller.initSipTables();
	controller.runWorker();
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
}

TEST(BasicSial,DISABLED_helloworld1){

	std::stringstream expected, output;
	expected << "my rank = " << attr->global_rank() << "\n";
	expected <<
			"hello world  this should be on the same line as hello world\n"
			"3.1415899999999998826  10044.5\n"
			"123\n"
			"456\n";
	output   << "my rank = " << attr->global_rank() << "\n";
	TestController controller("helloworld",false, VERBOSE_TEST, "this test should print \"hello world\" and some other numbers", output);
	controller.initSipTables();
	controller.runWorker();
	if (attr->global_rank() == 0) {ASSERT_EQ(expected.str(), output.str());}
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
}

TEST(BasicSial,DISABLED_scalars){
	std::string job("scalars");
	std::string comment("this test checks scalar initialization");
	double x = 3.456;
	double y = -0.1;
	std::stringstream expected, output;
	expected << "my rank = " << attr->global_rank() << "\n";
	output   << "my rank = " << attr->global_rank() << "\n";
	expected << "9:  x=3.456\n" <<
			"10:  y=-0.1\n" <<
			"14:  z=3.456\n\n" <<
			"15:  zz=99.99\n\n" <<
			"e should be 6\n" <<
			"22:  e=6\n\n";

	//create .dat file
	if (attr->global_rank() == 0) {
		init_setup(job.c_str());
		set_scalar("x", x);
		set_scalar("y", y);
		std::string tmp = job + ".siox";
		const char* nm = tmp.c_str();
		add_sial_program(nm);
		finalize_setup();
	}
	TestController controller(job, true, VERBOSE_TEST, comment, output);
	controller.initSipTables();
	controller.runWorker();
	if (attr->global_rank() == 0) {ASSERT_EQ(expected.str(), output.str());}
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
}




TEST(BasicSial,DISABLED_no_arg_user_sub) {
	std::string job("no_arg_user_sub");
	TestController controller(job, false, VERBOSE_TEST, "", std::cout);
	controller.initSipTables();
	controller.runWorker();
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
}


TEST(BasicSial,DISABLED_index_decs) {
	std::string job("index_decs");

	//set up index_decs.dat file
	if (attr->global_rank() == 0){
		init_setup(job.c_str());
		//now add data
		set_constant("norb",15);
		int segs[] = {5,6,7,8};
		set_aoindex_info(4,segs);
		//add the first program for this job and finalize
		std::string tmp = job + ".siox";
		const char* nm= tmp.c_str();
		add_sial_program(nm);
		finalize_setup();
	}

    barrier();
	TestController controller(job, true, VERBOSE_TEST, "sial program is only declarations, does not execute anything", std::cout);
	barrier();
	controller.initSipTables();
	//check some properties of the sip tables.
	int i_index_slot = controller.sip_tables_->index_id("i");
	int j_index_slot = controller.sip_tables_->index_id("j");
	int aio_index_slot = controller.sip_tables_->index_id("aoi");
	EXPECT_EQ(1, controller.sip_tables_->lower_seg(i_index_slot));
	EXPECT_EQ(4, controller.sip_tables_->num_segments(i_index_slot));

	EXPECT_EQ(4, controller.sip_tables_->lower_seg(j_index_slot));
	EXPECT_EQ(2, controller.sip_tables_->num_segments(j_index_slot));

	EXPECT_EQ(1, controller.sip_tables_->lower_seg(aio_index_slot));
	EXPECT_EQ(15, controller.sip_tables_->num_segments(aio_index_slot));

	//interpret the program
	controller.runWorker();
}



TEST(BasicSial,where_clause){
	std::string job("where_clause");
	std::stringstream out;
	TestController controller(job, false, VERBOSE_TEST, "", out);
	controller.initSipTables();
	controller.runWorker();
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
	int counter = controller.int_value("counter");
	EXPECT_DOUBLE_EQ(10, counter);
	}



TEST(BasicSial,ifelse){
	std::string job("ifelse");
	std::stringstream out;
	TestController controller(job, false, VERBOSE_TEST, "", out);
	controller.initSipTables();
	controller.runWorker();
	EXPECT_TRUE(controller.worker_->all_stacks_empty());
	int eq_counter = controller.int_value("eq_counter");
	int neq_counter = controller.int_value("neq_counter");
	EXPECT_EQ(4, eq_counter);
	EXPECT_EQ(20, neq_counter);
	}














//
//
//
///**  This is a template for a test  with no .dat file and no servers */
//TEST(BasicSial,DISABLED_empty) {
//	std::string job("empty");  /* REPLACE WITH TEST NAME */
//if (VERBOSE_TEST){
//	std::cout << "**************** STARTING TEST " << job
//			<< " ***********************\n"
//	        << std::flush;
//}
//
//	//no setup file, but the siox readers expects a SetupReader, so create an empty one.
//	setup::SetupReader* setup_reader = setup::SetupReader::get_empty_reader();
//	//read .siox file
//	std::string prog_name = job + ".siox";
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables* sipTables = new sip::SipTables(*setup_reader, siox_file);
//
//if (VERBOSE_TEST){
//	//rank 0 prints and .siox files contents
//	if (attr->global_rank() == 0) {
//		std::cout << "JOBNAME = " << job << std::endl << std::flush;
//		std::cout << "SETUP READER DATA:\n" << *setup_reader << std::endl
//				<< std::flush;
//		std::cout << "SIP TABLES" << '\n' << *sipTables << std::endl
//				<< std::flush;
//		std::cout
//				/* REPLACE WITH COMMENT FOR USER ABOUT TEST */
//				<< "COMMENT:  This is an empty program.  It should simply terminate.\n\n"
//				<< std::endl << std::flush;
//	}
//}
//    delete setup_reader;
//
//	barrier();
////	sip::WorkerPersistentArrayManager wpam;
//	{
////		sip::SialxTimer sialxTimer(sipTables->max_timer_slots());
//		sip::SialPrinter* printer_ = new sip::SialPrinterForTests(std::cout, attr->global_rank());
//		sip::Interpreter runner(*sipTables, printer_);
//
//		barrier();
//		if (VERBOSE_TEST) std::cout << "Rank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " STARTING" << std::endl << std::flush;
//		runner.interpret();
//		if (VERBOSE_TEST) std::cout << "\nRank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " TERMINATED" << std::endl << std::flush;
//		barrier();
//		/* CHECK WORKER STATE HERE */
//		EXPECT_TRUE(runner.all_stacks_empty());
//	}
//	if (VERBOSE_TEST) std::cout << "\nRank " << attr->global_rank() << " TEST " << job
//			<< " TERMINATED" << std::endl << std::flush;
//	delete sipTables;
//}
//
//TEST(BasicSial,DISABLED_literals) {
//	std::string job("empty");  /* REPLACE WITH TEST NAME */
//if (VERBOSE_TEST){
//	std::cout << "**************** STARTING TEST " << job
//			<< " ***********************\n"
//	        << std::flush;
//}
//
//	//no setup file, but the siox readers expects a SetupReader, so create an empty one.
//	setup::SetupReader &setup_reader = *setup::SetupReader::get_empty_reader();
//	//read .siox file
//	std::string prog_name = job + ".siox";
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//if (VERBOSE_TEST){
//	//rank 0 prints and .siox files contents
//	if (attr->global_rank() == 0) {
//		std::cout << "JOBNAME = " << job << std::endl << std::flush;
//		std::cout << "SETUP READER DATA:\n" << setup_reader << std::endl
//				<< std::flush;
//		std::cout << "SIP TABLES" << '\n' << sipTables << std::endl
//				<< std::flush;
//		std::cout
//				/* REPLACE WITH COMMENT FOR USER ABOUT TEST */
//				<< "COMMENT:  This is an empty program.  It should simply terminate.\n\n"
//				<< std::endl << std::flush;
//	}
//}
//
//	barrier();
//	sip::WorkerPersistentArrayManager wpam;
//	{
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer);
//		barrier();
//		if (VERBOSE_TEST) std::cout << "Rank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " STARTING" << std::endl << std::flush;
//		runner.interpret();
//		if (VERBOSE_TEST) std::cout << "\nRank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " TERMINATED" << std::endl << std::flush;
//		barrier();
//		/* CHECK WORKER STATE HERE */
//		EXPECT_TRUE(runner.all_stacks_empty());
//	}
//	if (VERBOSE_TEST) std::cout << "\nRank " << attr->global_rank() << " TEST " << job
//			<< " TERMINATED" << std::endl << std::flush;
//}
//
///** This test is a template for test-created .dat file with no servers */
//TEST(BasicSial,DISABLED_scalars) {
//	std::string job("scalars");  /* INSERT TEST NAME HERE */
//	if (VERBOSE_TEST) std::cout << "**************** STARTING TEST " << job
//			<< " ***********************\n"
//	        << std::flush;
//
//	//initialize variables used in test
//	sip::DataManager::scope_count = 0;
//	double x = 3.456;
//	double y = -0.1;
//	std::stringstream expected, output;
//	expected << "my rank = " << attr->global_rank() << "\n";
//	output   << "my rank = " << attr->global_rank() << "\n";
//	expected << "x = 3.4559999999999999609 at line 9\n" <<
//			"y = -0.10000000000000000555 at line 10\n" <<
//			"z = 3.4559999999999999609 at line 14\n" <<
//			"zz = 99.989999999999994884 at line 15\n" <<
//			"e should be 6\n" <<
//			"e = 6 at line 22\n";
//
//
//	//create .dat file
//	if (attr->global_rank() == 0) {
//		init_setup(job.c_str());
//		set_scalar("x", x);
//		set_scalar("y", y);
//		std::string tmp = job + ".siox";
//		const char* nm = tmp.c_str();
//		add_sial_program(nm);
//		finalize_setup();
//	}
//
//	barrier();
//
//    //read .dat file into setup_reader object
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//    //get siox name from setup_reader and load sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	if (VERBOSE_TEST)
//	//rank 0 prints .dat and .siox files contents
//	if (attr->global_rank() == 0) {
//		std::cout << "JOBNAME = " << job << std::endl << std::flush;
//		std::cout << "SETUP READER DATA:\n" << setup_reader << std::endl
//				<< std::flush;
//		std::cout << "SIP TABLES" << '\n' << sipTables << std::endl
//				<< std::flush;
//	}
//
//	barrier();
//	sip::WorkerPersistentArrayManager wpam;
////	#ifdef HAVE_MPI
////		sip::ServerPersistentArrayManager spam;
////
////		sip::DataDistribution data_distribution(sipTables, *attr);
////		sip::GlobalState::set_program_name(prog_name);
////		sip::GlobalState::increment_program();
////		if (attr->is_server()){
////			sip::SIPServer server(sipTables, data_distribution, *attr, &spam);
////			barrier();
////			std::cout << "Rank " << attr->global_rank() << " SERVER " << job << " STARTING"<< std::endl << std::flush;
////			server.run();
////			std::cout << "\nRank " << attr->global_rank() <<" SERVER " << job << " TERMINATED"<< std::endl << std::flush;
////			barrier();
////	      //CHECK SERVER STATE HERE
////		} else
////#endif
////	interpret the program
//	{
////		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
////		sip::Interpreter runner(sipTables, sialxTimer, output);
//		sip::SialPrinter* printer_ = new sip::SialPrinterForTests(output, attr->global_rank());
//		sip::Interpreter runner(sipTables, printer_);
//		barrier();
//		if (VERBOSE_TEST) std::cout << "Rank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " STARTING" << std::endl << std::flush;
//		runner.interpret();
//		if (VERBOSE_TEST){
//			std::cout << output.str();
//			std::cout << "\nRank " << attr->global_rank() << " SIAL PROGRAM " << job
//				<< " TERMINATED" << std::endl << std::flush;
//		}
//		barrier();
//		//CHECK WORKER STATE
//		ASSERT_DOUBLE_EQ(x, scalar_value("x"));
//		ASSERT_DOUBLE_EQ(y, scalar_value("y"));
//		ASSERT_DOUBLE_EQ(x, scalar_value("z"));
//		ASSERT_DOUBLE_EQ(99.99, scalar_value("zz"));
//		ASSERT_EQ(0, sip::DataManager::scope_count);
//		if (attr->global_rank() == 0) {ASSERT_EQ(expected.str(), output.str());}
//	}
//
//}
//TEST(SimpleMPI,Simple_Put_Test){
//	std::cout << "****************************************\n";
//	sip::GlobalState::reset_program_count();
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("put_test");
//	std::cout << "JOBNAME = " << job << std::endl;
//	int norb = 2;
//
//
//
//	if (attr->global_rank() == 0){
//		init_setup(job.c_str());
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm = tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {3, 4};
//		set_aoindex_info(2,segs);
//		finalize_setup();
//	}
//
//	barrier();
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//	std::cout << "SIP TABLES" << '\n' << sipTables << std::endl;
//
//
//	//interpret the program
//
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	sip::WorkerPersistentArrayManager wpam;
//
//#ifdef HAVE_MPI
//	sip::ServerPersistentArrayManager spam;
//
//	sip::DataDistribution data_distribution(sipTables, *attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (attr->is_server()){
//		sip::SIPServer server(sipTables, data_distribution, *attr, &spam);
//		server.run();
//
//	} else {
//#endif
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer, &wpam);
//		runner.interpret();
//		ASSERT_EQ(0, sip::DataManager::scope_count);
//
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//
//		int x_slot = sip::Interpreter::global_interpreter->array_slot(std::string("x"));
//		int y_slot = sip::Interpreter::global_interpreter->array_slot(std::string("y"));
//		sip::index_selector_t x_indices, y_indices;
//		for (int i = 0; i < MAX_RANK; i++) x_indices[i] = y_indices[i] = sip::unused_index_value;
//		x_indices[0] = y_indices[0] = 1;
//		x_indices[1] = y_indices[0] = 1;
//		sip::BlockId x_bid(x_slot, x_indices);
//		sip::BlockId y_bid(y_slot, y_indices);
//
//		sip::Block::BlockPtr x_bptr = sip::Interpreter::global_interpreter->get_block_for_reading(x_bid);
//		sip::Block::dataPtr x_data = x_bptr->get_data();
//		sip::Block::BlockPtr y_bptr = sip::Interpreter::global_interpreter->get_block_for_reading(y_bid);
//		sip::Block::dataPtr y_data = y_bptr->get_data();
//
//		for (int i=0; i<3*3; i++){
//			ASSERT_EQ(1, x_data[i]);
//			ASSERT_EQ(2, y_data[i]);
//		}
//#ifdef HAVE_MPI
//		}
//#endif
//}

//// Sanity test to check for no compiler errors, crashes, etc.
//TEST(SimpleMPI,persistent_empty_mpi){
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//	int my_rank = sip_mpi_attr.global_rank();
//
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("persistent_empty_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + "1.siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		std::string tmp1 = job + "2.siox";
//		const char* nm1= tmp1.c_str();
//		add_sial_program(nm1);
//		int segs[]  = {2,3,4,1};
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	//read and print setup_file
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	sip::WorkerPersistentArrayManager wpam;
//	sip::ServerPersistentArrayManager spam;
//
//	//Execute first program
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//	if (!sip_mpi_attr.is_server()) {std::cout << "SIP TABLES for " << prog_name << '\n' << sipTables << std::endl;}
//
//	//create worker and server
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\n>>>>>>>>>>>>starting SIAL PROGRAM  "<< job << std::endl;}
//
//	std::cout << "rank " << my_rank << " reached first barrier in test" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed first barrier in test" << std::endl << std::flush;
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, &spam);
//		std::cout << "reached prog 1 server barrier" << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"passed prog 1 server barrier" << std::endl;
//		server.run();
//		spam.save_marked_arrays(&server);
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer, &wpam);
//		std::cout << "reached prg 1 worker barrier " << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "passed prog 1 worker barrier "  << std::endl << std::flush;
//		runner.interpret();
//		wpam.save_marked_arrays(&runner);
//	}
//
//   std::cout << std::flush;
//   if (sip_mpi_attr.global_rank()==0){
//	   std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl << std::flush;
//	   std::cout << "SETUP READER DATA FOR SECOND PROGRAM:\n" << setup_reader<< std::endl;
//   }
//
//	std::string prog_name2 = setup_reader.sial_prog_list_.at(1);
//	setup::BinaryInputFile siox_file2(siox_dir + prog_name2);
//	sip::SipTables sipTables2(setup_reader, siox_file2);
//	if (sip_mpi_attr.global_rank()==0){
//		std::cout << "SIP TABLES FOR " << prog_name2 << '\n' << sipTables2 << std::endl;
//	}
//	sip::DataDistribution data_distribution2(sipTables2, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	std::cout << "rank " << my_rank << " reached second barrier in test" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed second barrier in test" << std::endl << std::flush;
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables2, data_distribution2, sip_mpi_attr, &spam);
//		std::cout << "reached prog 2 server barrier " << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"passed prog 2 server barrier" << std::endl<< std::flush;
//		server.run();
//		spam.save_marked_arrays(&server);
//	} else {
//		sip::SialxTimer sialxTimers(sipTables2.max_timer_slots());
//		sip::Interpreter runner(sipTables2, sialxTimers, &wpam);
//		std::cout << "reached prog 2 worker barrier " << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "passed prog 2 worker barrier"<< job  << std::endl<< std::flush;
//		runner.interpret();
//		wpam.save_marked_arrays(&runner);
//		std::cout << "\nSIAL PROGRAM 2 TERMINATED"<< std::endl;
//	}
//
//	std::cout << "rank " << my_rank << " reached third barrier in test" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed third barrier in test" << std::endl << std::flush;
//
//}
//
//TEST(SimpleMPI,persistent_distributed_array_mpi){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//	int my_rank = sip_mpi_attr.global_rank();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("persistent_distributed_array_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 2;
//	int segs[]  = {2,3};
//
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + "1.siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		std::string tmp1 = job + "2.siox";
//		const char* nm1= tmp1.c_str();
//		add_sial_program(nm1);
//		set_aoindex_info(2,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//	if (!sip_mpi_attr.is_server()) {std::cout << "SIP TABLES" << '\n' << sipTables << std::endl;}
//
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\n>>>>>>>>>>>>starting SIAL PROGRAM  "<< job << std::endl;}
//
//	//create worker and server
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	sip::WorkerPersistentArrayManager wpam;
//	sip::ServerPersistentArrayManager spam;
//
//	std::cout << "rank " << my_rank << " reached first barrier" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed first barrier" << std::endl << std::flush;
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, &spam);
//		std::cout << "at first barrier in prog 1 at server" << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"passed first barrier at server, starting server" << std::endl;
//		server.run();
//		spam.save_marked_arrays(&server);
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  &wpam);
//		std::cout << "at first barrier in prog 1 at worker" << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "after first barrier; starting worker for "<< job  << std::endl;
//		runner.interpret();
//		wpam.save_marked_arrays(&runner);
//		std::cout << "\n end of prog1 at worker"<< std::endl;
//
//	}
//
//	std::cout << std::flush;
//	if (sip_mpi_attr.global_rank()==0){
//		std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl << std::flush;
//		std::cout << "SETUP READER DATA FOR SECOND PROGRAM:\n" << setup_reader<< std::endl;
//	}
//
//	std::string prog_name2 = setup_reader.sial_prog_list_.at(1);
//	setup::BinaryInputFile siox_file2(siox_dir + prog_name2);
//	sip::SipTables sipTables2(setup_reader, siox_file2);
//
//	if (sip_mpi_attr.global_rank()==0){
//		std::cout << "SIP TABLES FOR " << prog_name2 << '\n' << sipTables2 << std::endl;
//	}
//
//	sip::DataDistribution data_distribution2(sipTables2, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	std::cout << "rank " << my_rank << " reached second barrier in test" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed second barrier in test" << std::endl << std::flush;
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables2, data_distribution2, sip_mpi_attr, &spam);
//		std::cout << "barrier in prog 2 at server" << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<< "rank " << my_rank << "starting server for prog 2" << std::endl;
//		server.run();
//		std::cout<< "rank " << my_rank  << "Server state after termination of prog2" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer2(sipTables2.max_timer_slots());
//		sip::Interpreter runner(sipTables2, sialxTimer2,  &wpam);
//		std::cout << "rank " << my_rank << "barrier in prog 2 at worker" << std::endl << std::flush;
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "rank " << my_rank << "starting worker for prog2"<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM 2 TERMINATED"<< std::endl;
//
//
//		// Test contents of blocks of distributed array "b"
//
//		// Get the data for local array block "b"
//		int b_slot = runner.array_slot(std::string("lb"));
//
//		// Test b(1,1)
//		sip::index_selector_t b_indices_1;
//		b_indices_1[0] = 1; b_indices_1[1] = 1;
//		for (int i = 2; i < MAX_RANK; i++) b_indices_1[i] = sip::unused_index_value;
//		sip::BlockId b_bid_1(b_slot, b_indices_1);
//		std::cout << b_bid_1 << std::endl;
//		sip::Block::BlockPtr b_bptr_1 = runner.get_block_for_reading(b_bid_1);
//		sip::Block::dataPtr b_data_1 = b_bptr_1->get_data();
//		std::cout << " Comparing block " << b_bid_1 << std::endl;
//		double fill_seq_1_1 = 1.0;
//		for (int i=0; i<segs[0]; i++){
//			for (int j=0; j<segs[0]; j++){
//				ASSERT_DOUBLE_EQ(fill_seq_1_1, b_data_1[i*segs[0] + j]);
//				fill_seq_1_1++;
//			}
//		}
//
//		// Test b(2, 2)
//		sip::index_selector_t b_indices_2;
//		b_indices_2[0] = 2; b_indices_2[1] = 2;
//		for (int i = 2; i < MAX_RANK; i++) b_indices_2[i] = sip::unused_index_value;
//		sip::BlockId b_bid_2(b_slot, b_indices_2);
//		std::cout << b_bid_2 << std::endl;
//		sip::Block::BlockPtr b_bptr_2 = runner.get_block_for_reading(b_bid_2);
//		sip::Block::dataPtr b_data_2 = b_bptr_2->get_data();
//		std::cout << " Comparing block " << b_bid_2 << std::endl;
//		double fill_seq_2_2 = 4.0;
//		for (int i=0; i<segs[1]; i++){
//			for (int j=0; j<segs[1]; j++){
//				ASSERT_DOUBLE_EQ(fill_seq_2_2, b_data_2[i*segs[1] + j]);
//				fill_seq_2_2++;
//			}
//		}
//
//		// Test b(2,1)
//		sip::index_selector_t b_indices_3;
//		b_indices_3[0] = 2; b_indices_3[1] = 1;
//		for (int i = 2; i < MAX_RANK; i++) b_indices_3[i] = sip::unused_index_value;
//		sip::BlockId b_bid_3(b_slot, b_indices_3);
//		std::cout << b_bid_3 << std::endl;
//		sip::Block::BlockPtr b_bptr_3 = runner.get_block_for_reading(b_bid_3);
//		sip::Block::dataPtr b_data_3 = b_bptr_3->get_data();
//		std::cout << " Comparing block " << b_bid_3 << std::endl;
//		double fill_seq_2_1 = 3.0;
//		for (int i=0; i<segs[1]; i++){
//			for (int j=0; j<segs[0]; j++){
//				ASSERT_DOUBLE_EQ(fill_seq_2_1, b_data_3[i*segs[0] + j]);
//				fill_seq_2_1++;
//			}
//		}
//
//
//	}
//
//	std::cout << "rank " << my_rank << " reached third barrier in test" << std::endl << std::flush;
//	MPI_Barrier(MPI_COMM_WORLD);
//	std::cout << "rank " << my_rank << " passed third barrier in test" << std::endl << std::flush;
//
//}
//
//
///************************************************/
//
//TEST(SimpleMPI,get_mpi){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("get_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//	int segs[]  = {2,3,4,1};
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//		list_block_map();
//
//		// Test a(1,1)
//		// Get the data for local array block "b"
//		int a_slot = runner.array_slot(std::string("a"));
//
//		sip::index_selector_t a_indices_1;
//		a_indices_1[0] = 1; a_indices_1[1] = 1;
//		for (int i = 2; i < MAX_RANK; i++) a_indices_1[i] = sip::unused_index_value;
//		sip::BlockId a_bid_1(a_slot, a_indices_1);
//		std::cout << a_bid_1 << std::endl;
//		sip::Block::BlockPtr a_bptr_1 = runner.get_block_for_reading(a_bid_1);
//		sip::Block::dataPtr a_data_1 = a_bptr_1->get_data();
//		std::cout << " Comparing block " << a_bid_1 << std::endl;
//		for (int i=0; i<segs[0]; i++){
//			for (int j=0; j<segs[0]; j++){
//				ASSERT_DOUBLE_EQ(42*3, a_data_1[i*segs[0] + j]);
//			}
//		}
//	}
//}
//
///************************************************/
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//TEST(SimpleMPI,unmatched_get){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("unmatched_get");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {2,3,4,1};
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//		list_block_map();
//	}
//	std::cout << "this test should have printed a warning about unmatched get";
//}
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//TEST(SimpleMPI,delete_mpi){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("delete_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {2,3,4,1};
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//	}
//}
//
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//// Tested in get_mpi
//TEST(SimpleMPI,put_accumulate_mpi){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("put_accumulate_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {2,3,4,1};
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	// setup_reader.read(setup_file);
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//	}
//}
//
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//// Tested in get_mpi
//TEST(SimpleMPI,put_test_mpi){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("put_test_mpi");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 4;
//
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {2,3,4,1};
//		set_aoindex_info(4,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	//create worker and server
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//	}
//}
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//// Prints to stdout.
//TEST(SimpleMPI,all_rank_print){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("all_rank_print_test");
//	std::cout << "JOBNAME = " << job << std::endl;
//	double x = 3.456;
//	int norb = 2;
//
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_scalar("x",x);
//		set_constant("norb",norb);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {2,3};
//		set_aoindex_info(2,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	//create worker and server
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//	}
//}
//
//
//// TODO ===========================================
//// TODO AUTOMATE
//// TODO ===========================================
//// Nothing should crash. Find something to test.
//TEST(SimpleMPI,Message_Number_Wraparound){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("message_number_wraparound_test");
//	std::cout << "JOBNAME = " << job << std::endl;
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_constant("norb",1);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {1};
//		set_aoindex_info(1,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	//create worker and server
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//	}
//}
//
//
//TEST(SimpleMPI,Pardo_Loop_Test){
//
//	sip::GlobalState::reset_program_count();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//
//	std::cout << "****************************************\n";
//	sip::DataManager::scope_count=0;
//	//create setup_file
//	std::string job("pardo_loop");
//	std::cout << "JOBNAME = " << job << std::endl;
//
//	if (sip_mpi_attr.global_rank() == 0){
//		init_setup(job.c_str());
//		set_constant("norb",1);
//		std::string tmp = job + ".siox";
//		const char* nm= tmp.c_str();
//		add_sial_program(nm);
//		int segs[]  = {1};
//		set_aoindex_info(1,segs);
//		finalize_setup();
//	}
//
//	sip::SIPMPIUtils::check_err(MPI_Barrier(MPI_COMM_WORLD));
//
//	setup::BinaryInputFile setup_file(job + ".dat");
//	setup::SetupReader setup_reader(setup_file);
//
//	std::cout << "SETUP READER DATA:\n" << setup_reader<< std::endl;
//
//	//get siox name from setup, load and print the sip tables
//	std::string prog_name = setup_reader.sial_prog_list_.at(0);
//	std::string siox_dir(dir_name);
//	setup::BinaryInputFile siox_file(siox_dir + prog_name);
//	sip::SipTables sipTables(setup_reader, siox_file);
//
//	//create worker and server
//	if (sip_mpi_attr.global_rank()==0){   std::cout << "\n\n\n\nstarting SIAL PROGRAM  "<< job << std::endl;}
//
//
//	sip::DataDistribution data_distribution(sipTables, sip_mpi_attr);
//	sip::GlobalState::set_program_name(prog_name);
//	sip::GlobalState::increment_program();
//	if (sip_mpi_attr.is_server()){
//		sip::SIPServer server(sipTables, data_distribution, sip_mpi_attr, NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout<<"starting server" << std::endl;
//		server.run();
//		std::cout << "Server state after termination" << server << std::endl;
//	} else {
//		sip::SialxTimer sialxTimer(sipTables.max_timer_slots());
//		sip::Interpreter runner(sipTables, sialxTimer,  NULL);
//		MPI_Barrier(MPI_COMM_WORLD);
//		std::cout << "starting worker for "<< job  << std::endl;
//		runner.interpret();
//		std::cout << "\nSIAL PROGRAM TERMINATED"<< std::endl;
//		ASSERT_DOUBLE_EQ(80, runner.data_manager_.scalar_value("total"));
//	}
//}
//
void bt_sighandler(int signum) {
	std::cerr << "Interrupt signal (" << signum << ") received." << std::endl;
    FAIL();
    abort();
}


int main(int argc, char **argv) {

//    feenableexcept(FE_DIVBYZERO);
//    feenableexcept(FE_OVERFLOW);
//    feenableexcept(FE_INVALID);
//
//    signal(SIGSEGV, bt_sighandler);
//    signal(SIGFPE, bt_sighandler);
//    signal(SIGTERM, bt_sighandler);
//    signal(SIGINT, bt_sighandler);
//    signal(SIGABRT, bt_sighandler);


#ifdef HAVE_MPI
	MPI_Init(&argc, &argv);
	int num_procs;
	sip::SIPMPIUtils::check_err(MPI_Comm_size(MPI_COMM_WORLD, &num_procs));

	if (num_procs < 2) {
		std::cerr << "Please run this test with at least 2 mpi ranks"
				<< std::endl;
		return -1;
	}
	sip::SIPMPIUtils::set_error_handler();
	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
	attr = &sip_mpi_attr;
#endif

#ifdef HAVE_TAU
	TAU_PROFILE_SET_NODE(0);
	TAU_STATIC_PHASE_START("SIP Main");
#endif

//	sip::check(sizeof(int) >= 4, "Size of integer should be 4 bytes or more");
//	sip::check(sizeof(double) >= 8, "Size of double should be 8 bytes or more");
//	sip::check(sizeof(long long) >= 8, "Size of long long should be 8 bytes or more");
//
//	int num_procs;
//	sip::SIPMPIUtils::check_err(MPI_Comm_size(MPI_COMM_WORLD, &num_procs));
//
//	if (num_procs < 2){
//		std::cerr<<"Please run this test with at least 2 mpi ranks"<<std::endl;
//		return -1;
//	}
//
//	sip::SIPMPIUtils::set_error_handler();
//	sip::SIPMPIAttr &sip_mpi_attr = sip::SIPMPIAttr::get_instance();
//

	printf("Running main() from test_simple.cpp\n");
	testing::InitGoogleTest(&argc, argv);
	int result = RUN_ALL_TESTS();

#ifdef HAVE_TAU
	TAU_STATIC_PHASE_STOP("SIP Main");
#endif
#ifdef HAVE_MPI
	MPI_Finalize();
#endif
	return result;

}
