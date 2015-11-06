/*
 * SearchNormal.cpp
 *
 *  Created on: 25 Oct 2015
 *      Author: hieu
 */

#include <algorithm>
#include <boost/foreach.hpp>
#include "SearchNormal.h"
#include "Stack.h"
#include "Manager.h"
#include "../InputPaths.h"
#include "../TargetPhrases.h"
#include "../TargetPhrase.h"
#include "../System.h"

using namespace std;

SearchNormal::SearchNormal(Manager &mgr, std::vector<Stack> &stacks)
:m_mgr(mgr)
,m_stacks(stacks)
,m_stackSize(mgr.GetSystem().stackSize)
{
	// TODO Auto-generated constructor stub

}

SearchNormal::~SearchNormal() {
	// TODO Auto-generated destructor stub
}

void SearchNormal::Decode(size_t stackInd)
{
  Stack &stack = m_stacks[stackInd];

  std::vector<const Hypothesis*> hypos = stack.GetBestHyposAndPrune(m_stackSize, m_mgr.GetHypoRecycle());
  BOOST_FOREACH(const Hypothesis *hypo, hypos) {
		Extend(*hypo);
  }
  DebugStacks();
}

void SearchNormal::Extend(const Hypothesis &hypo)
{
	const InputPaths &paths = m_mgr.GetInputPaths();

	BOOST_FOREACH(const InputPath &path, paths) {
		Extend(hypo, path);
	}
}

void SearchNormal::Extend(const Hypothesis &hypo, const InputPath &path)
{
	const Moses::Bitmap &bitmap = hypo.GetBitmap();
	const Moses::Range &hypoRange = hypo.GetRange();
	const Moses::Range &pathRange = path.GetRange();

	if (bitmap.Overlap(pathRange)) {
		return;
	}

	int distortion = abs((int)pathRange.GetStartPos() - (int)hypoRange.GetEndPos() - 1);
	if (distortion > 5) {
		return;
	}

	const Moses::Bitmap &newBitmap = m_mgr.GetBitmaps().GetBitmap(bitmap, pathRange);

	const std::vector<TargetPhrases::shared_const_ptr> &tpsAllPt = path.GetTargetPhrases();

	for (size_t i = 0; i < tpsAllPt.size(); ++i) {
		const TargetPhrases *tps = tpsAllPt[i].get();
		if (tps) {
			Extend(hypo, *tps, pathRange, newBitmap);
		}
	}
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrases &tps,
		const Moses::Range &pathRange,
		const Moses::Bitmap &newBitmap)
{
  BOOST_FOREACH(const TargetPhrase *tp, tps) {
	  Extend(hypo, *tp, pathRange, newBitmap);
  }
}

void SearchNormal::Extend(const Hypothesis &hypo,
		const TargetPhrase &tp,
		const Moses::Range &pathRange,
		const Moses::Bitmap &newBitmap)
{
	Hypothesis *newHypo = Hypothesis::Create(m_mgr);
	newHypo->Init(hypo, tp, pathRange, newBitmap);
	newHypo->EvaluateWhenApplied();

	size_t numWordsCovered = newBitmap.GetNumWordsCovered();
	Stack &stack = m_stacks[numWordsCovered];
	StackAdd added = stack.Add(newHypo);

	Recycler<Hypothesis*> &hypoRecycle = m_mgr.GetHypoRecycle();

	if (added.added) {
		// we're winners!
		if (added.other) {
			// there was a existing losing hypo
			hypoRecycle.push(added.other);
		}
	}
	else {
		// we're losers!
		// there should be a winner, we're not doing beam pruning
		UTIL_THROW_IF2(added.other == NULL, "There must have been a winning hypo");
		hypoRecycle.push(newHypo);
	}

	//m_arcLists.AddArc(stackAdded.added, newHypo, stackAdded.other);
}

void SearchNormal::DebugStacks() const
{
	  BOOST_FOREACH(const Stack &stack, m_stacks) {
		  cerr << stack.GetSize() << " ";
	  }
	  cerr << endl;
}

const Hypothesis *SearchNormal::GetBestHypothesis() const
{
	const Stack &lastStack = m_stacks.back();
	std::vector<const Hypothesis*> sortedHypos = lastStack.GetBestHypos(1);

	const Hypothesis *best = NULL;
	if (sortedHypos.size()) {
		best = sortedHypos[0];
	}
	return best;
}


