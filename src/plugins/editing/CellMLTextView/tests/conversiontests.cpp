/*******************************************************************************

Copyright (C) The University of Auckland

OpenCOR is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenCOR is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

*******************************************************************************/

//==============================================================================
// CellML Text view conversion tests
//==============================================================================

#include "../../../../tests/src/testsutils.h"

//==============================================================================

#include "cellmlfile.h"
#include "cellmltextviewconverter.h"
#include "cellmltextviewparser.h"
#include "conversiontests.h"
#include "corecliutils.h"

//==============================================================================

#include <QtTest/QtTest>

//==============================================================================

void ConversionTests::successfulConversionTests()
{
    OpenCOR::CellMLTextView::CellMLTextViewConverter converter;

    // Test the conversion of a CellML file that works with COR...

    QStringList cellmlCorCellmlContents = OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_cor.cellml"));

    QVERIFY(converter.execute(cellmlCorCellmlContents.join('\n')));
    QCOMPARE(converter.output().split('\n'),
             OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_cor.out")));

    // ...and back

    OpenCOR::CellMLTextView::CellmlTextViewParser parser;

    QVERIFY(parser.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_cor.out")).join('\n'),
                           OpenCOR::CellMLSupport::CellmlFile::Cellml_1_0));
    QVERIFY(!parser.domDocument().isNull());

    QCOMPARE(QString(OpenCOR::Core::serialiseDomDocument(parser.domDocument())).split('\n'),
             cellmlCorCellmlContents);

    // Test the conversion of a CellML file that only works with OpenCOR

    QVERIFY(converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_opencor.cellml")).join('\n')));
    QCOMPARE(converter.output().split('\n'),
             OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_opencor.out")));

    // Retest the conversion of a CellML file that works with COR, except that
    // we insert comments everywhere

    static const QString Comment = QString("<!-- In between comment #%1...-->");

    QStringList cellmlCorWithCommentsCellmlContents = QStringList() << cellmlCorCellmlContents[0];
    QString currentLine;
    int commentNumber = 0;
    int i = 0;

    forever {
        currentLine = cellmlCorCellmlContents[++i];

        cellmlCorWithCommentsCellmlContents << currentLine;

        if (currentLine.compare("</model>"))
            cellmlCorWithCommentsCellmlContents << Comment.arg(++commentNumber);
        else
            break;
    }

    QVERIFY(converter.execute(cellmlCorWithCommentsCellmlContents.join('\n')));
    QCOMPARE(converter.output().split('\n'),
             OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/successful/cellml_cor_with_comments.out")));
}

//==============================================================================

void ConversionTests::failingConversionTests()
{
    // Test the conversion of various invalid CellML files

    OpenCOR::CellMLTextView::CellMLTextViewConverter converter;

    // CellML base units

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/cellml_base_units.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'base_units' attribute must have a value of 'yes' or 'no'."));
    QCOMPARE(converter.errorLine(), 3);
    QCOMPARE(converter.errorColumn(), 59);

    // CellML reaction

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/cellml_reaction.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'reaction' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));
    QCOMPARE(converter.errorLine(), 4);
    QCOMPARE(converter.errorColumn(), 15);

    // CellML group

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/cellml_relationship.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'relationship' attribute in the CellML namespace must have a value of 'encapsulation' or 'containment'."));
    QCOMPARE(converter.errorLine(), 4);
    QCOMPARE(converter.errorColumn(), 79);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/cellml_relationship_ref.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'relationship_ref' element with a 'relationship' attribute value of 'encapsulation' must not define a 'name' attribute."));
    QCOMPARE(converter.errorLine(), 4);
    QCOMPARE(converter.errorColumn(), 72);

    // CellML connection

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/cellml_connection.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'connection' element must contain exactly one 'map_components' element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 74);

    // MathML token elements

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'cn' element must have a value."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'cn' element with an 'e-notation' type must have three child elements."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 67);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The first child element of a 'cn' element with an 'e-notation' type must be of 'text' type."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 67);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_3.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The second child element of a 'cn' element with an 'e-notation' type must be of 'element' type."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 67);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_4.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The name of the second child element of a 'cn' element with an 'e-notation' type must be 'sep'."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 67);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_5.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The third child element of a 'cn' element with an 'e-notation' type must be of 'text' type."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 67);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_unsupported_type.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The 'cn' element uses a 'complex-polar' type that is unsupported."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 70);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cn_e_notation_unknown_type.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The 'cn' element uses a 'some-unknown-type' type that is unknown."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 74);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_ci.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'ci' element must have a value."));
    QCOMPARE(converter.errorLine(), 7);
    QCOMPARE(converter.errorColumn(), 20);

    // MathML basic content elements

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_apply.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'apply' element must have at least one child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 19);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_piecewise_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'piecewise' element can only be used within a top-level 'apply' element that has an 'eq' element as its first child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_piecewise_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'piecewise' element cannot be used within another 'piecewise' element."));
    QCOMPARE(converter.errorLine(), 10);
    QCOMPARE(converter.errorColumn(), 26);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_piecewise_3.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'piecewise' element must have at least one child element."));
    QCOMPARE(converter.errorLine(), 8);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_piece.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'piece' element must have two child elements."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 19);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_otherwise.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'otherwise' element must have one child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 23);

    // MathML relational operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_eq.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'eq' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_neq.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'neq' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_gt.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'gt' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_lt.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'lt' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_geq.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'geq' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_leq.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'leq' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    // MathML arithmetic operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_plus.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'plus' element must have at least one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 19);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_minus.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'minus' element must have at least one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_times.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'times' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_divide.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'divide' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_power.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'power' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_rem.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'rem' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_root_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'root' element must have either one or two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 19);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_root_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The first sibling of a 'root' element with two siblings must be a 'degree' element."));
    QCOMPARE(converter.errorLine(), 7);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_abs.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'abs' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_exp.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'exp' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_ln.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'ln' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_log.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'log' element must have either one or two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_ceiling.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'ceiling' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_floor.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'floor' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 20);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_factorial.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'factorial' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    // MathML logical operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_and.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'and' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_or.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'or' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_xor.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'xor' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_not.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'not' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 18);

    // MathML calculus elements

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_diff_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'diff' element must have two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 19);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_diff_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The first sibling of a 'diff' element with two siblings must be a 'bvar' element."));
    QCOMPARE(converter.errorLine(), 7);
    QCOMPARE(converter.errorColumn(), 20);

    // MathML qualifier elements

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_degree.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'degree' element must have one child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 17);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_logbase.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'logbase' element must have one child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 18);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_bvar_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'bvar' element must have one or two child elements."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 15);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_bvar_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("The second child element of a 'bvar' element with two child elements must be a 'degree' element."));
    QCOMPARE(converter.errorLine(), 9);
    QCOMPARE(converter.errorColumn(), 20);

    // MathML min/max operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_min.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'min' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_max.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'max' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    // MathML gcd/lcm operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_gcd.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'gcd' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_lcm.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'lcm' element must have at least two siblings."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    // MathML trigonometric operators

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_sin.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'sin' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cos.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'cos' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_tan.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'tan' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_sec.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'sec' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_csc.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'csc' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cot.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'cot' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 21);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_sinh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'sinh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_cosh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'cosh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_tanh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'tanh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_sech.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'sech' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_csch.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'csch' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_coth.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'coth' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 22);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arcsin.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arcsin' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccos.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccos' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arctan.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arctan' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arcsec.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arcsec' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccsc.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccsc' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccot.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccot' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 24);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arcsinh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arcsinh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccosh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccosh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arctanh.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arctanh' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arcsech.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arcsech' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccsch.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccsch' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_arccoth.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("An 'arccoth' element must have one sibling."));
    QCOMPARE(converter.errorLine(), 6);
    QCOMPARE(converter.errorColumn(), 25);

    // MathML constants

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_constant_1.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'pi' element cannot have a child element."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 16);

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_constant_2.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'pi' element cannot have child elements."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 16);

    // Unknown MathML element

    QVERIFY(!converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/failing/mathml_unknown_element.cellml")).join('\n')));
    QCOMPARE(converter.errorMessage(),
             QString("A 'unknown_element' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));
    QCOMPARE(converter.errorLine(), 5);
    QCOMPARE(converter.errorColumn(), 29);
}

//==============================================================================

void ConversionTests::warningConversionTests()
{
    // Unknown CellML element

    OpenCOR::CellMLTextView::CellMLTextViewConverter converter;

    QVERIFY(converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/warning/cellml_unknown_element.cellml")).join('\n')));
    QCOMPARE(converter.warnings().count(), 1);
    QCOMPARE(converter.warnings().first().message(),
             QString("A 'unknown_element' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));
    QCOMPARE(converter.warnings().first().lineNumber(), 3);
    QCOMPARE(converter.warnings().first().columnNumber(), 21);

    // Known, but unsupported MathML elements

    QVERIFY(converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/warning/mathml_semantics.cellml")).join('\n')));
    QCOMPARE(converter.warnings().count(), 1);
    QCOMPARE(converter.warnings().first().message(),
             QString("A 'semantics' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));
    QCOMPARE(converter.warnings().first().lineNumber(), 5);
    QCOMPARE(converter.warnings().first().columnNumber(), 23);

    QVERIFY(converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/warning/mathml_annotation.cellml")).join('\n')));
    QCOMPARE(converter.warnings().count(), 1);
    QCOMPARE(converter.warnings().first().lineNumber(), 5);
    QCOMPARE(converter.warnings().first().columnNumber(), 24);
    QCOMPARE(converter.warnings().first().message(),
             QString("An 'annotation' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));

    QVERIFY(converter.execute(OpenCOR::fileContents(OpenCOR::fileName("src/plugins/editing/CellMLTextView/tests/data/conversion/warning/mathml_annotation-xml.cellml")).join('\n')));
    QCOMPARE(converter.warnings().count(), 1);
    QCOMPARE(converter.warnings().first().message(),
             QString("An 'annotation-xml' element was found in the original CellML file, but it is not supported and cannot therefore be processed."));
    QCOMPARE(converter.warnings().first().lineNumber(), 5);
    QCOMPARE(converter.warnings().first().columnNumber(), 28);
}

//==============================================================================

QTEST_GUILESS_MAIN(ConversionTests)

//==============================================================================
// End of file
//==============================================================================
