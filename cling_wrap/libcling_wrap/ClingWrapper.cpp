/*
 * ClingWrapper.cpp
 *
 *  Created on: Jul 28, 2014
 *      Author: Stephan Opfer
 */

#include "ClingWrapper.h"

#include "BaseLiteral.h"
#include "BoolLiteral.h"
#include "External.h"

#include "clasp/solver.h"

using namespace Clasp;
using namespace Clasp::Cli;

namespace supplementary
{

	ClingWrapper::ClingWrapper()
	{
		setMode(Mode::mode_clingo);
		this->setVerbose(0);
		this->claspAppOpts_.quiet[0] = 2;
		this->claspAppOpts_.quiet[1] = 2;
		this->claspAppOpts_.quiet[2] = 2;
		this->grOpts_.lparseDebug = Gringo::Output::LparseDebug::NONE;

	}

	ClingWrapper::~ClingWrapper()
	{
		// TODO Auto-generated destructor stub
	}

	void ClingWrapper::init()
	{
		this->setup();
		bool incremental = mode_ != mode_clasp;
		Clasp::ProblemType pt = getProblemType();
		ProgramBuilder* prg = &clasp_->start(claspConfig_, pt, incremental);
		if (incremental)
		{
			grOpts_.verbose = verbose() == UINT_MAX;
			Asp::LogicProgram* lp = mode_ != mode_gringo ? static_cast<Asp::LogicProgram*>(prg) : 0;
			grd = Gringo::make_unique<Grounder>();
			grd->parse(claspAppOpts_.input, grOpts_, lp, this); // <- hier dafür sorgen, dass die clasp output classe ersetzt wird
			this->ground("base", {});
		}
	}

	void ClingWrapper::setMode(Mode mode)
	{
		this->mode_ = mode;
	}

	Gringo::SolveResult ClingWrapper::solve()
	{
		lastModel = nullptr;
		lastSolver = nullptr;
		Gringo::SolveResult result = ClingoApp::solve(nullptr);
		return result;
	}

	void ClingWrapper::ground(std::string const &name, Gringo::FWValVec args)
	{
		ClingoApp::ground(name, args);
	}

	void ClingWrapper::addKnowledgeFile(std::string path)
	{
		this->claspAppOpts_.input.push_back(path);
	}

	bool ClingWrapper::onModel(const Clasp::Solver& s, const Clasp::Model& m)
	{
		lastModel = &m;
		lastSolver = &s;

		for (auto baseLit : this->baseLiterals)
                {
		  if (baseLit->getUpdateType() == LiteralUpdateType::PUSH)
                    baseLit->updateOnModel(&s, &m);
                }

		return true;
	}


        int ClingWrapper::getNumWildcards(Gringo::Value &value)
        {
          if (value.type() != Gringo::Value::Type::FUNC)
             return 0;

          int count = 0;

          for (uint i = 0; i < value.args().size(); ++i)
          {
            Gringo::Value arg = value.args()[i];

            if (arg.type() == Gringo::Value::Type::STRING && arg.name() == "?")
              ++count;
          }

          return count;
        }

	void ClingWrapper::assignExternal(shared_ptr<Gringo::Value> ext, bool val)
	{
		ClingoApp::assignExternal(*ext, val);
	}

	shared_ptr<Gringo::Value>ClingWrapper::ClingWrapper::assignExternal(const std::string& name, Gringo::FWValVec args, bool val)
	{
		shared_ptr<Gringo::Value> gv = (args.size() == 0 ? make_shared<Gringo::Value>(name) : make_shared<Gringo::Value>(name, args));
		assignExternal(gv, val);
		return gv;
	}

	void ClingWrapper::releaseExternal(shared_ptr<Gringo::Value> ext)
	{
		ClingoApp::releaseExternal(*ext);
	}

	shared_ptr<Gringo::Value> ClingWrapper::releaseExternal(std::string const &name, Gringo::FWValVec args)
	{
		shared_ptr<Gringo::Value> gv = (args.size() == 0 ? make_shared<Gringo::Value>(name) : make_shared<Gringo::Value>(name, args));
		releaseExternal (gv);
		return gv;
	}

	void ClingWrapper::add(const std::string& name, const Gringo::FWStringVec& params, const std::string& part)
	{
		ClingoApp::add(name, params, part);
	}

	void ClingWrapper::printLastModel(bool verbose)
	{
	        std::cout << "Last Model ";

		if (!lastModel)
		{
			std::cout << "No model found" << std::endl;
			return;
		}

		for (auto value : this->lastSolver->symbolTable())
		{
			if (this->lastModel->isTrue(value.second.lit))
				std::cout << value.second.name.c_str() << " ";
		}

		std::cout << std::endl;

		if (verbose)
		{
		        std::cout << "Symbol table: ";
		        for (auto value : this->lastSolver->symbolTable())
                        {
                                std::cout << "(" << value.second.name.c_str() << ", " << value.first << ", " <<
                                    (this->lastModel->isTrue(value.second.lit) ? "true" : "false") << ") ";
                        }

	                std::cout << std::endl;
		}
	}

        std::shared_ptr<External> const ClingWrapper::getExternal(const std::string& name, Gringo::FWValVec args,
                                                                  const std::string &ground)
        {
          return this->getExternal(name, args, ground, args);
        }

	std::shared_ptr<External> const ClingWrapper::getExternal(const std::string& name, Gringo::FWValVec args,
	                                                          const std::string &ground, Gringo::FWValVec groundArgs)
	{
	  auto value = std::make_shared<Gringo::Value>(name, args);

	  try
	  {
	    return this->externals.at(value->hash());
	  }
	  catch (std::out_of_range &e)
	  {
	    std::lock_guard<std::mutex> guard (this->mutexExternals);

            try
            {
              return this->externals.at(value->hash());
            }
            catch (std::out_of_range &e)
            {
              External* ptr = new External(this, value);
              auto external = std::shared_ptr<External>(ptr);
              this->externals.insert(std::pair<size_t, std::shared_ptr<External>>(value->hash(), external));

              this->ground(ground, groundArgs);

              return external;
            }
	  }
	}

        void ClingWrapper::registerLiteral(unsigned int literal, Gringo::Value value)
        {
          std::lock_guard<std::mutex> guard (this->mutexLiterals);
          this->existingLiterals.push_back(std::tuple<unsigned int, Gringo::Value>(literal, value));

          for (auto baseLit : this->baseLiterals)
          {
            if (baseLit->getCheckNewLiterals())
              baseLit->check(literal, value);
          }

//          std::cout << "Symbol Muh " << literal << ", ";
//          value.print(std::cout);
//          std::cout << std::endl;
        }

        std::shared_ptr<BoolLiteral> const ClingWrapper::getBoolLiteral(std::string const &name, Gringo::FWValVec args)
        {
          std::lock_guard<std::mutex> guard (this->mutexLiterals);

          Gringo::Value query(name, args);

          for (auto lit : this->baseLiterals)
          {
            if (lit->getType() == LiteralType::BOOL && lit->getQuery() == query)
            {
                return std::static_pointer_cast<BoolLiteral>(lit);
            }
          }

          int wildcards = this->getNumWildcards(query);

          if (wildcards != 0)
          {
            throw std::exception();
          }

          auto boolLiteral = std::make_shared<BoolLiteral>(this, query);
          this->baseLiterals.push_back(boolLiteral);

          for (auto lit : this->existingLiterals)
          {
            boolLiteral->check(std::get<0>(lit), std::get<1>(lit));
          }

          return boolLiteral;
        }

        const Clasp::Model* ClingWrapper::getLastModel()
        {
          return this->lastModel;
        }

        const Clasp::Solver* ClingWrapper::getLastSolver()
        {
          return this->lastSolver;
        }

} /* namespace supplementary */
