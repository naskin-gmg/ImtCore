// SPDX-License-Identifier: LGPL-2.1-or-later OR GPL-2.0-or-later OR GPL-3.0-or-later OR LicenseRef-ImtCore-Commercial
#include <imtclientgql/CGqlRemoteRepresentationControllerCompBase.h>


// ImtCore includes
#include <imtgql/CGqlResponse.h>


namespace imtclientgql
{


// protected methods

// reimplemented (imtservergql::CGqlRepresentationDataControllerComp)

imtbase::CTreeItemModel* CGqlRemoteRepresentationControllerCompBase::CreateInternalResponse(const imtgql::CGqlRequest& gqlRequest, QString& errorMessage) const
{
	if (!IsRequestSupported(gqlRequest)){
		return nullptr;
	}

	if (!m_apiClientCompPtr.IsValid()){
		SendCriticalMessage(0, "Component 'ApiClient' was not set. Check the component configuration");

		return nullptr;
	}

	imtclientgql::IGqlClient::GqlRequestPtr requestPtr;
	requestPtr.MoveCastedPtr(gqlRequest.CloneMe());
	if (requestPtr.IsValid()){
		imtclientgql::IGqlClient::GqlResponsePtr responsePtr = m_apiClientCompPtr->SendRequest(requestPtr);
		if (responsePtr.IsValid()){
			return CreateTreeItemModelFromResponse(gqlRequest.GetCommandId(), *responsePtr);
		}

		errorMessage = QString("Command could not be processed by the remote server: '%1'").arg(qPrintable(requestPtr->GetCommandId()));
	}

	return nullptr;
}


// private methods

imtbase::CTreeItemModel* CGqlRemoteRepresentationControllerCompBase::CreateTreeItemModelFromResponse(
			const QByteArray& commandId,
			const imtgql::IGqlResponse& response) const
{
	istd::TDelPtr<imtbase::CTreeItemModel> retVal(new imtbase::CTreeItemModel());
	QByteArray responseData = response.GetResponseData();
	QJsonDocument document = QJsonDocument::fromJson(responseData);
	if (document.isObject()){
		QJsonObject dataObject = document.object();

		imtgql::IGqlResponse::GqlRequestPtr requestPtr = response.GetOriginalRequest();
		if (!requestPtr.IsValid()){
			return nullptr;
		}

		// --- Errors (new format: array) ---
		if (dataObject.contains("errors")){
			QJsonValue jsonValue = dataObject.value("errors");
			if (jsonValue.isArray()){
				QJsonArray errorArray = jsonValue.toArray();
				if (!errorArray.isEmpty()){
					QJsonValue firstError = errorArray.first();
					dataObject = QJsonObject();
					dataObject.insert("errors", firstError);
					document.setObject(dataObject);
					QByteArray parserData = document.toJson(QJsonDocument::Compact);
					retVal->CreateFromJson(parserData);
				}
			}
		}

		// --- Data ---
		dataObject = document.object().value("data").toObject();
		if (!dataObject.isEmpty()){
			QJsonValue bodyValue = dataObject.value(requestPtr->GetCommandId());
			if (!bodyValue.isNull()){
				dataObject = QJsonObject();
				dataObject.insert("data", bodyValue);
				document.setObject(dataObject);
				QByteArray parserData = document.toJson(QJsonDocument::Compact);
				retVal->CreateFromJson(parserData);
			}
		}

		// --- WebSocket / subscription payloads ---
		if (document.object().value("type").toString() == "query_data"){
			QJsonObject payloadObject = document.object().value("payload").toObject();

			QJsonValue dataValue;
			QString type = "data";

			if (payloadObject.contains("data")){
				QJsonObject payloadData = payloadObject.value("data").toObject();
				dataValue = payloadData.value(commandId);
			}

			if (payloadObject.contains("errors")){
				type = "errors";
				QJsonValue payloadErrors = payloadObject.value("errors");

				if (payloadErrors.isArray()){
					QJsonArray errorArray = payloadErrors.toArray();
					if (!errorArray.isEmpty()){
						dataValue = errorArray.first();
					}
				}
			}

			if (!dataValue.isNull()){
				QJsonObject newDataObject;
				newDataObject.insert(type, dataValue);
				document.setObject(newDataObject);
				QByteArray parserData = document.toJson(QJsonDocument::Compact);
				retVal->CreateFromJson(parserData);
			}
		}
		else{
			dataObject = document.object().value("payload").toObject();
			if (!dataObject.isEmpty()){
				document.setObject(dataObject);
				QByteArray parserData = document.toJson(QJsonDocument::Compact);
				retVal->CreateFromJson(parserData);
			}
		}
	}

	return retVal.PopPtr();
}


} // namespace imtclientgql


