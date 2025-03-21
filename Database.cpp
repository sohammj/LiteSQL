#include "Database.h"
#include "Utils.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

// Create table
void Database::createTable(const std::string& tableName,
    const std::vector<std::pair<std::string, std::string>>& cols) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) != tables.end()) {
        std::cout << "Error: Table " << tableName << " already exists." << std::endl;
        return;
    }
    Table table;
    for (const auto& col : cols) {
        table.addColumn(col.first, col.second);
    }
    tables[lowerName] = table;
    std::cout << "Table " << tableName << " created." << std::endl;
}

void Database::dropTable(const std::string& tableName) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.erase(lowerName))
        std::cout << "Table " << tableName << " dropped." << std::endl;
    else
        std::cout << "Table " << tableName << " does not exist." << std::endl;
}

void Database::alterTableAddColumn(const std::string& tableName, const std::pair<std::string, std::string>& column) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    tables[lowerName].addColumn(column.first, column.second);
    std::cout << "Column " << column.first << " added to " << tableName << "." << std::endl;
}

void Database::alterTableDropColumn(const std::string& tableName, const std::string& columnName) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    bool success = tables[lowerName].dropColumn(columnName);
    if (success)
        std::cout << "Column " << columnName << " dropped from " << tableName << "." << std::endl;
    else
        std::cout << "Column " << columnName << " does not exist in " << tableName << "." << std::endl;
}

void Database::describeTable(const std::string& tableName) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    std::cout << "Schema for " << tableName << ":" << std::endl;
    const auto& cols = tables[lowerName].getColumns();
    for (const auto& col : cols)
        std::cout << col << "\t";
    std::cout << std::endl;
}

void Database::insertRecord(const std::string& tableName,
                              const std::vector<std::vector<std::string>>& values) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    for (const auto& valueSet : values) {
        tables[lowerName].addRow(valueSet);
    }
    std::cout << "Record(s) inserted into " << tableName << "." << std::endl;
}

void Database::selectRecords(const std::string& tableName,
                             const std::vector<std::string>& selectColumns,
                             const std::string& condition,
                             const std::vector<std::string>& orderByColumns,
                             const std::vector<std::string>& groupByColumns,
                             const std::string& havingCondition,
                             bool isJoin,
                             const std::string& joinTable,
                             const std::string& joinCondition) {
    if (!isJoin) {
        std::string lowerName = toLowerCase(tableName);
        if (tables.find(lowerName) == tables.end()) {
            std::cout << "Table " << tableName << " does not exist." << std::endl;
            return;
        }
        tables[lowerName].selectRows(selectColumns, condition, orderByColumns, groupByColumns, havingCondition);
    } else {
        // JOIN implementation (nested-loop inner join)
        std::string leftName = toLowerCase(tableName);
        std::string rightName = toLowerCase(joinTable);
        if (tables.find(leftName) == tables.end() || tables.find(rightName) == tables.end()) {
            std::cout << "One or both tables in JOIN do not exist." << std::endl;
            return;
        }
        size_t eqPos = joinCondition.find('=');
        if (eqPos == std::string::npos) {
            std::cout << "Invalid join condition." << std::endl;
            return;
        }
        std::string leftJoin = trim(joinCondition.substr(0, eqPos));
        std::string rightJoin = trim(joinCondition.substr(eqPos + 1));
        size_t dotPos = leftJoin.find('.');
        if (dotPos != std::string::npos)
            leftJoin = leftJoin.substr(dotPos + 1);
        dotPos = rightJoin.find('.');
        if (dotPos != std::string::npos)
            rightJoin = rightJoin.substr(dotPos + 1);
        
        Table& leftTable = tables[leftName];
        Table& rightTable = tables[rightName];
        const auto& leftCols = leftTable.getColumns();
        const auto& rightCols = rightTable.getColumns();
        auto lit = std::find(leftCols.begin(), leftCols.end(), leftJoin);
        auto rit = std::find(rightCols.begin(), rightCols.end(), rightJoin);
        if (lit == leftCols.end() || rit == rightCols.end()) {
            std::cout << "Join columns not found." << std::endl;
            return;
        }
        int leftIdx = std::distance(leftCols.begin(), lit);
        int rightIdx = std::distance(rightCols.begin(), rit);
        
        std::vector<std::vector<std::string>> joinResult;
        std::vector<std::string> combinedHeader = leftCols;
        for (const auto& col : rightCols)
            combinedHeader.push_back(col);
        
        const auto& leftRows = leftTable.getRows();
        const auto& rightRows = rightTable.getRows();
        for (const auto& lrow : leftRows) {
            for (const auto& rrow : rightRows) {
                if (lrow[leftIdx] == rrow[rightIdx]) {
                    std::vector<std::string> combinedRow = lrow;
                    combinedRow.insert(combinedRow.end(), rrow.begin(), rrow.end());
                    joinResult.push_back(combinedRow);
                }
            }
        }
        
        std::vector<std::string> displayColumns;
        if (selectColumns.size() == 1 && selectColumns[0] == "*")
            displayColumns = combinedHeader;
        else
            displayColumns = selectColumns;
        
        std::vector<std::string> processedDisplayColumns;
        for (const auto& col : displayColumns) {
            std::string processed = col;
            size_t dotPos = processed.find('.');
            if (dotPos != std::string::npos)
                processed = processed.substr(dotPos + 1);
            processedDisplayColumns.push_back(processed);
        }
        
        for (const auto& col : displayColumns)
            std::cout << col << "\t";
        std::cout << std::endl;
        
        for (const auto& row : joinResult) {
            for (const auto& procCol : processedDisplayColumns) {
                auto it = std::find(combinedHeader.begin(), combinedHeader.end(), procCol);
                if (it != combinedHeader.end()) {
                    int idx = std::distance(combinedHeader.begin(), it);
                    std::cout << row[idx] << "\t";
                }
            }
            std::cout << std::endl;
        }
    }
}

void Database::deleteRecords(const std::string& tableName, const std::string& condition) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    tables[lowerName].deleteRows(condition);
    std::cout << "Records deleted from " << tableName << "." << std::endl;
}

void Database::updateRecords(const std::string& tableName,
                             const std::vector<std::pair<std::string, std::string>>& updates,
                             const std::string& condition) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    tables[lowerName].updateRows(updates, condition);
    std::cout << "Records updated in " << tableName << "." << std::endl;
}

void Database::showTables() {
    std::cout << "Available Tables:" << std::endl;
    for (const auto& pair : tables)
        std::cout << pair.first << std::endl;
}

// Transaction functions
void Database::beginTransaction() {
    if (!inTransaction) {
        backupTables = tables;
        inTransaction = true;
        std::cout << "Transaction started." << std::endl;
    } else {
        std::cout << "Transaction already in progress." << std::endl;
    }
}

void Database::commitTransaction() {
    if (!inTransaction) {
       std::cout << "No active transaction to commit." << std::endl;
       return;
    }
    inTransaction = false;
    backupTables.clear();
    std::cout << "Transaction committed." << std::endl;
}

void Database::rollbackTransaction() {
    if (!inTransaction) {
       std::cout << "No active transaction to rollback." << std::endl;
       return;
    }
    tables = backupTables;
    backupTables.clear();
    inTransaction = false;
    std::cout << "Transaction rolled back." << std::endl;
}

// New functionalities

void Database::truncateTable(const std::string& tableName) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    tables[lowerName].clearRows();
    std::cout << "Table " << tableName << " truncated." << std::endl;
}

void Database::renameTable(const std::string& oldName, const std::string& newName) {
    std::string lowerOld = toLowerCase(oldName);
    std::string lowerNew = toLowerCase(newName);
    if (tables.find(lowerOld) == tables.end()) {
        std::cout << "Table " << oldName << " does not exist." << std::endl;
        return;
    }
    tables[lowerNew] = tables[lowerOld];
    tables.erase(lowerOld);
    std::cout << "Table " << oldName << " renamed to " << newName << "." << std::endl;
}

void Database::createIndex(const std::string& indexName, const std::string& tableName, const std::string& columnName) {
    std::string lowerTable = toLowerCase(tableName);
    if (tables.find(lowerTable) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    indexes[toLowerCase(indexName)] = {lowerTable, columnName};
    std::cout << "Index " << indexName << " created on " << tableName << "(" << columnName << ")." << std::endl;
}

void Database::dropIndex(const std::string& indexName) {
    std::string lowerIndex = toLowerCase(indexName);
    if (indexes.erase(lowerIndex))
        std::cout << "Index " << indexName << " dropped." << std::endl;
    else
        std::cout << "Index " << indexName << " does not exist." << std::endl;
}

void Database::mergeRecords(const std::string& tableName, const std::string& mergeCommand) {
    // --- Step 1: Locate key clauses ---
    // Expected syntax:
    // MERGE INTO tableName USING (SELECT ... AS col, ... ) AS src
    // ON tableName.col = src.col
    // WHEN MATCHED THEN UPDATE SET col = <value>, ...
    // WHEN NOT MATCHED THEN INSERT VALUES (<value>, ...);
    std::string commandUpper = toUpperCase(mergeCommand);
    size_t usingPos = commandUpper.find("USING");
    size_t onPos = commandUpper.find("ON");
    size_t whenMatchedPos = commandUpper.find("WHEN MATCHED THEN UPDATE SET");
    size_t whenNotMatchedPos = commandUpper.find("WHEN NOT MATCHED THEN INSERT VALUES");
    
    if (usingPos == std::string::npos || onPos == std::string::npos ||
        whenMatchedPos == std::string::npos || whenNotMatchedPos == std::string::npos) {
        std::cout << "MERGE: Invalid MERGE syntax." << std::endl;
        return;
    }
    
    // --- Step 2: Extract the source subquery from the USING clause ---
    size_t sourceStart = mergeCommand.find("(", usingPos);
    size_t sourceEnd = mergeCommand.find(")", sourceStart);
    if (sourceStart == std::string::npos || sourceEnd == std::string::npos) {
        std::cout << "MERGE: Invalid source subquery syntax." << std::endl;
        return;
    }
    std::string sourceSubquery = mergeCommand.substr(sourceStart + 1, sourceEnd - sourceStart - 1);
    // Expect the subquery to start with SELECT.
    size_t selectPos = toUpperCase(sourceSubquery).find("SELECT");
    if (selectPos == std::string::npos) {
        std::cout << "MERGE: Source subquery must start with SELECT." << std::endl;
        return;
    }
    std::string selectExpressions = sourceSubquery.substr(selectPos + 6);
    // (For simplicity, we assume the SELECT returns a comma‑separated list of expressions)
    std::vector<std::string> exprList = split(selectExpressions, ',');
    // Build a source record map: alias (lowercase) -> literal value.
    std::unordered_map<std::string, std::string> srcRecord;
    for (const auto &expr : exprList) {
        std::string trimmedExpr = trim(expr);
        size_t asPos = toUpperCase(trimmedExpr).find("AS");
        if (asPos == std::string::npos)
            continue;
        std::string literal = trim(trimmedExpr.substr(0, asPos));
        std::string alias = trim(trimmedExpr.substr(asPos + 2));
        // Remove quotes if the literal is quoted.
        if (!literal.empty() && literal.front() == '\'' && literal.back() == '\'' && literal.size() > 1)
            literal = literal.substr(1, literal.size() - 2);
        srcRecord[toLowerCase(alias)] = literal;
    }
    
    // --- Step 3: Parse the ON clause ---
    // Expected format: ON tableName.col = src.col
    std::string onClause = mergeCommand.substr(onPos, whenMatchedPos - onPos);
    // Remove the leading "ON"
    onClause = trim(onClause.substr(2));
    size_t eqPos = onClause.find('=');
    if (eqPos == std::string::npos) {
        std::cout << "MERGE: Invalid ON clause." << std::endl;
        return;
    }
    std::string targetExpr = trim(onClause.substr(0, eqPos));
    std::string srcExpr = trim(onClause.substr(eqPos + 1));
    size_t dotPos = targetExpr.find('.');
    std::string targetColumn = (dotPos != std::string::npos) ? toLowerCase(trim(targetExpr.substr(dotPos + 1))) : toLowerCase(targetExpr);
    dotPos = srcExpr.find('.');
    std::string srcColumn = (dotPos != std::string::npos) ? toLowerCase(trim(srcExpr.substr(dotPos + 1))) : toLowerCase(srcExpr);
    
    // --- Step 4: Parse the UPDATE clause (WHEN MATCHED) ---
    size_t updateClauseStart = whenMatchedPos + std::string("WHEN MATCHED THEN UPDATE SET").length();
    std::string updateClause = mergeCommand.substr(updateClauseStart, whenNotMatchedPos - updateClauseStart);
    updateClause = trim(updateClause);
    // Split assignments by commas.
    std::unordered_map<std::string, std::string> updateAssignments;
    {
        std::vector<std::string> assignments = split(updateClause, ',');
        for (const auto &assign : assignments) {
            size_t eqAssign = assign.find('=');
            if (eqAssign == std::string::npos)
                continue;
            std::string col = toLowerCase(trim(assign.substr(0, eqAssign)));
            std::string val = trim(assign.substr(eqAssign + 1));
            // Replace references like "src.col" with the corresponding source value.
            if (toUpperCase(val).find("SRC.") != std::string::npos) {
                size_t pos = toUpperCase(val).find("SRC.");
                std::string refCol = toLowerCase(trim(val.substr(pos + 4)));
                if (srcRecord.find(refCol) != srcRecord.end()) {
                    val = srcRecord[refCol];
                }
            } else if (!val.empty() && val.front() == '\'' && val.back() == '\'' && val.size() > 1) {
                val = val.substr(1, val.size() - 2);
            }
            updateAssignments[col] = val;
        }
    }
    
    // --- Step 5: Parse the INSERT clause (WHEN NOT MATCHED) ---
    size_t insertClauseStart = whenNotMatchedPos + std::string("WHEN NOT MATCHED THEN INSERT VALUES").length();
    std::string insertClause = mergeCommand.substr(insertClauseStart);
    insertClause = trim(insertClause);
    // Remove surrounding parentheses if present.
    if (!insertClause.empty() && insertClause.front() == '(' && insertClause.back() == ')') {
        insertClause = insertClause.substr(1, insertClause.size() - 2);
        insertClause = trim(insertClause);
    }
    std::vector<std::string> insertValues = split(insertClause, ',');
    for (auto &val : insertValues) {
        val = trim(val);
        if (toUpperCase(val).find("SRC.") != std::string::npos) {
            size_t pos = toUpperCase(val).find("SRC.");
            std::string refCol = toLowerCase(trim(val.substr(pos + 4)));
            if (srcRecord.find(refCol) != srcRecord.end())
                val = srcRecord[refCol];
        } else if (!val.empty() && val.front() == '\'' && val.back() == '\'' && val.size() > 1) {
            val = val.substr(1, val.size() - 2);
        }
    }
    
    // --- Step 6: Apply the MERGE to the target table ---
    std::string lowerTable = toLowerCase(tableName);
    if (tables.find(lowerTable) == tables.end()) {
        std::cout << "MERGE: Table " << tableName << " does not exist." << std::endl;
        return;
    }
    const auto& targetCols = tables[lowerTable].getColumns();
    // Find the index of the target column in the table.
    int targetIndex = -1;
    for (size_t i = 0; i < targetCols.size(); i++) {
        if (toLowerCase(targetCols[i]) == targetColumn) {
            targetIndex = i;
            break;
        }
    }
    if (targetIndex == -1) {
        std::cout << "MERGE: Target column " << targetColumn << " not found in table." << std::endl;
        return;
    }
    
    bool matched = false;
    // Check each row for a match on the ON condition.
    for (auto &row : tables[lowerTable].getRowsNonConst()) {
        if (row.size() > targetIndex && toLowerCase(row[targetIndex]) == toLowerCase(srcRecord[srcColumn])) {
            // When matched, update the row using the UPDATE assignments.
            for (size_t j = 0; j < targetCols.size(); j++) {
                std::string colName = toLowerCase(targetCols[j]);
                if (updateAssignments.find(colName) != updateAssignments.end()) {
                    row[j] = updateAssignments[colName];
                }
            }
            matched = true;
        }
    }
    
    if (!matched) {
        // No matching row found; build a new row using the INSERT values.
        // For simplicity, assume the number of insert values equals the number of columns.
        std::vector<std::string> newRow;
        for (size_t i = 0; i < targetCols.size(); i++) {
            if (i < insertValues.size())
                newRow.push_back(insertValues[i]);
            else
                newRow.push_back("");
        }
        tables[lowerTable].addRow(newRow);
    }
    
    std::cout << "MERGE command executed on " << tableName << "." << std::endl;
}


void Database::replaceInto(const std::string& tableName, const std::vector<std::vector<std::string>>& values) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    for (const auto& row : values) {
        bool replaced = false;
        for (auto& existingRow : tables[lowerName].getRowsNonConst()) {
            if (!existingRow.empty() && !row.empty() && existingRow[0] == row[0]) {
                existingRow = row;
                replaced = true;
                break;
            }
        }
        if (!replaced) {
            tables[lowerName].addRow(row);
        }
    }
    std::cout << "REPLACE INTO executed on " << tableName << "." << std::endl;
}

void Database::sortPhotos(const std::string& tableName, const std::string& column, bool ascending) {
    std::string lowerName = toLowerCase(tableName);
    if (tables.find(lowerName) == tables.end()) {
        std::cout << "Table " << tableName << " does not exist." << std::endl;
        return;
    }
    tables[lowerName].sortRows(column, ascending);
    std::cout << "Photos sorted in " << tableName << "." << std::endl;
}

void Database::trackRecentPhoto(const std::string& filename) {
    recentPhotos.push(filename);
    std::cout << "Photo " << filename << " tracked as recent." << std::endl;
}

void Database::showRecentPhotos() {
    std::cout << "Recent Photos:" << std::endl;
    while (!recentPhotos.empty()) {
        std::cout << recentPhotos.top() << std::endl;
        recentPhotos.pop();
    }
}