#ifndef HSEIR_HPP
#define HSEIR_HPP

#include "cohortseir.hpp"
#include "estimableseirfilter.hpp"

class hpartial: public partialseir
{
public:

    enum states { E,  Ia,  Ip,  Is, Iu, numactives, R=numactives,

                  Edelta, Iadelta, Ipdelta,  Isdelta,  Rdelta,
                  Hdelta, Rhdelta,
                  Ddelta, Dhdelta,

                  Isd, Rd,
                  Hd, Rhd,
                  Dd, Dhd,

                  numstates};

    enum params { alpha, sigma, varsigma, gammas, gammaa, iotas, mus,
                  theta, eta, pi,
                  gammah, muh,
                  prebeta,
                  betafactor,
                  numparams
                };

    hpartial() : partialseir()
    {}
    virtual dmatrix P(const vector<double>& params) const
    {
        /// more intutive to construct transposition
        dmatrix Pt(k(),k());
        Pt.setZero();

        Pt(E,Ia) = params[sigma] * params[alpha];
        Pt(E,Ip) = params[sigma] * (1-params[alpha]);
        Pt(Ip,Is) = params[varsigma] * params[pi];

Pt(Ip,Hd) = params[iotas];

        Pt(Ip,Iu) = params[varsigma] * (1-params[pi]);
        Pt(Ia,R) = params[gammaa];
        Pt(Is,R) = params[gammas];
        Pt(Iu,R) = params[gammas];

        Pt(E,Edelta) = params[theta];
        Pt(Ia,Iadelta) = params[theta];
        Pt(Ip,Ipdelta) = params[theta];
        Pt(Is,Isd) = params[eta];
        Pt(Is,Hd) = params[iotas];
        Pt(Is,Dd) = params[mus];
        Pt(Iu,Hd) = params[iotas];
        Pt(Iu,Dd) = params[mus];

        Pt(Edelta,Iadelta) = params[sigma] * params[alpha];
        Pt(Edelta,Ipdelta) = params[sigma] * (1-params[alpha]);
        Pt(Ipdelta,Isdelta) = params[varsigma];
        Pt(Iadelta,Rdelta) = params[gammaa];
        Pt(Isdelta,Rdelta) = params[gammas];
        Pt(Isdelta,Ddelta) = params[mus];
        Pt(Isdelta,Hdelta) = params[iotas];
        Pt(Hdelta,Rhdelta) = params[gammah];
        Pt(Hdelta,Dhdelta) = params[muh];

        Pt(Isd,Rd) = params[gammas];
        Pt(Isd,Dd) = params[mus];
        Pt(Isd,Hd) = params[iotas];
        Pt(Hd,Rhd) = params[gammah];
        Pt(Hd,Dhd) = params[muh];

        for(unsigned i=0; i<k(); i++)
        {
            double s = 0;
            for(unsigned j=0; j<k(); j++)
                s += Pt(i,j);
            Pt(i,i)=max(0.0,1-s);
        }

        if(0)
        {
            for(unsigned i=0; i< k(); i++)
                for(unsigned j=0; j< k(); j++)
                    if(Pt(i,j) < 0 || Pt(i,j) > 1)
                    {
                        cerr << m2csv(Pt) << endl;
                        throw "not a probability matrix";
                    }
        }
        return Pt.transpose();
    }


    virtual dmatrix hatB(const vector<double>& params) const
    {
        dmatrix Bt(k(),k());
        Bt.setZero();
        double b = params[prebeta]*params[betafactor];
        Bt(Is,E) = b;
        Bt(Ip,E) = b;
        Bt(Ia,E) = b / 4;
        Bt(Iu,E) = b;

        return Bt.transpose();
    }

    std::string statelabel(unsigned i) const
    {
        static std::vector<std::string> r =
        { "E", "Ia", "Ip" ,"Is","Iu","R",
          "Edelta","Iadelta","Ipdelta","Isdelta","Rdelta",
          "Hdelta","Rhdelta",
          "Ddelta","Dhdelta",
          "Isd","Rd",
          "Hd","Rhd",
          "Dd","Dhd" };
        assert(i<r.size());
        return r[i];
    }

    virtual unsigned k() const
    {
        return numstates;
    }

    virtual double susceptible( double population, const dvector&  compartments ) const
    {
        double s=0;
        for(unsigned i=0; i<compartments.size(); i++)
            s += compartments[i];
        assert(s < population);
        return 1.0 - s / population;
    }

};

class hcohortseir: public cohortseir<hpartial>
{
public:
    enum excolumns {I0,	I20,	I65,	I80,
                     DAYADJUST,PDET,REDUCTIONMEAN,REDUCTIONMEDIAN,FEAR,
                     numexcolumns};

    enum computationparams {
               omega,
               pi,
               thetacoef,
               eta0,
               theta0,
               numcomputationparams
    };

    hcohortseir(unsigned numcohorts, const vector<double>& pop) :
        cohortseir<hpartial>(numcohorts,pop)
    {
    }

    virtual std::vector<std::vector<double>> pars2pars(
            unsigned t, const vector<double>& params, const G& g) const
    {
        static std::vector<hpartial::params> commonpars
                = {hpartial::alpha,
                   hpartial::sigma,
                   hpartial::varsigma};

        static std::vector<hpartial::params> exclusivepars
               = { hpartial::betafactor,
                   hpartial::gammas,
                   hpartial::gammaa,
                   hpartial::iotas,
                   hpartial::mus,
                   hpartial::gammah,
                   hpartial::muh};

        static std::vector<hpartial::params> computedpars
               = { hpartial::pi,
                   hpartial::eta,
                   hpartial::theta,
                   hpartial::prebeta };

        assert(hpartial::numparams==
                                computedpars.size()
                               +commonpars.size()
                               +exclusivepars.size());


        assert(params.size()== numcomputationparams
                              + commonpars.size()
                              + numcohorts() * exclusivepars.size());

        std::vector<std::vector<double>>
           res(numcohorts());

        vector<double> preparams(hpartial::numparams,0);

        preparams[hpartial::pi] = params[pi] * g.z[t][PDET];

        preparams[hpartial::eta] =
                params[eta0]
                *  g.z[t][DAYADJUST];
        preparams[hpartial::theta] =
                (params[theta0] + params[thetacoef] * g.z[t][PDET])
                * g.z[t][DAYADJUST];;

        preparams[hpartial::prebeta]
           = g.z[t][REDUCTIONMEAN]
//                * exp(-params[omega] * g.z[t][FEAR] );
* (1-params[omega] * g.z[t][FEAR]);

        unsigned srcc=numcomputationparams;
        for(unsigned i=0; i<commonpars.size(); i++)
            preparams[commonpars[i]]=params[srcc++];

        for(unsigned c=0; c<numcohorts(); c++)
        {
            res[c] = preparams;
            for(unsigned i=0; i<exclusivepars.size(); i++)
                res[c][exclusivepars[i]] = params[srcc++];
        }
        assert(srcc==params.size());
        return res;
    }
};

class datareader
{
public:
    enum obscolumns {R0,	R20,	R65,	R80,
                     D0,	D20,	D65,	D80,
                     HY,	HO,
                     RHY,	RHO,
                     DHY,	DHO,
                     DOY,	DOO,
                     RA,	RS,
                      numobscolumns};

    virtual vector<string> obslabels() const = 0;

    virtual unsigned numobs() const = 0;
    virtual vector<double> obs2obs(const vector<double>&) const = 0;

    seirdata read(csv<','>& c, unsigned lag = 0)
    {
        seirdata res;
        bool rfin = false;

        auto lbls = obslabels();
        for(unsigned j=0; j<numobs(); j++)
            res.ylabels.push_back(lbls[j]);

        for(unsigned j=0; j<hcohortseir::numexcolumns; j++)
            res.zlabels.push_back(c(0,numobscolumns+j+1));

//        vector<double> lastobs(numobscolumns,0);

        for(unsigned i=lag; i< c.r()-1; i++)
        {
            res.dates.push_back(c(i+1,0));
            if(c(i+1,1) != "" && !rfin)
            {
                vector<double> origobs;
                for(unsigned j=0; j<numobscolumns; j++)
                    origobs.push_back( /* lastobs[j] */ +c.getdouble(i+1,1+j));
                auto obs = obs2obs(origobs);
                assert(obs.size()==numobs());
                res.y.push_back(dv(obs));
                // lastobs = origobs;
            }
            else
                rfin = true;
            vector<double> exp;
            for(unsigned j=0; j<hcohortseir::numexcolumns; j++)
                exp.push_back(c.getdouble(i+1,1+numobscolumns+j));
            res.z.push_back(dv(exp));
        }
        res.lag = lag;
        return res;
    }
};


template <typename S, bool wls>
class hestseir : public S, public estimableseirfilter<wls>
{
public:
    hestseir(const uncertain& ax0 = uncertain(dvector(0))) : S(),
        estimableseirfilter<wls>(),
        fx0(ax0)
    {}
    virtual uncertain X0(const vector<double>& params) const
    {
        if(fx0.x().size()==0)
            return seirfilter::X0(params);
        else
        {
            assert(fx0.x().size() == k());
            return fx0;

/*
            double is= params[initscale];
            auto x= fx0.x();
            auto V = fx0.var();
            auto a = hpartial::numactives;
            for(unsigned i=0; i<a; i++)
                x[i] *= is;
            return { x, block(V.block(0,0,a,a)*is*is,
                              V.block(0,a,a,k()-a) * is,
                              V.block(a,0,k()-a,a) * is,
                              V.block(a,a,k()-a,k()-a)) }; */

        }
    }


private:
    uncertain fx0;
};




#endif // HSEIR_HPP
